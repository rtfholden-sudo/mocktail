#include "runtime/domain_allowlist.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace mocktail {
namespace runtime {

namespace {

char LowerAscii(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

std::string NormalizeDomain(std::string_view entry) {
  std::string domain;
  domain.reserve(entry.size());
  for (const char c : entry) {
    domain.push_back(LowerAscii(c));
  }
  // Strip whitespace.
  const auto first = domain.find_first_not_of(" \t\r\n");
  const auto last = domain.find_last_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  domain = domain.substr(first, last - first + 1);
  // Strip a leading "*." wildcard and a trailing dot.
  if (domain.rfind("*.", 0) == 0) {
    domain.erase(0, 2);
  }
  while (!domain.empty() && domain.back() == '.') {
    domain.pop_back();
  }
  return domain;
}

}  // namespace

bool IsValidDomainEntry(std::string_view entry) {
  const std::string domain = NormalizeDomain(entry);
  if (domain.empty() || domain.size() > 253) {
    return false;
  }
  std::size_t label_start = 0;
  for (std::size_t i = 0; i <= domain.size(); ++i) {
    if (i == domain.size() || domain[i] == '.') {
      const std::size_t label_length = i - label_start;
      if (label_length == 0 || label_length > 63) {
        return false;
      }
      if (domain[label_start] == '-' || domain[i - 1] == '-') {
        return false;
      }
      label_start = i + 1;
    } else if (!std::isalnum(static_cast<unsigned char>(domain[i])) &&
               domain[i] != '-') {
      return false;
    }
  }
  return true;
}

bool IsValidDomainAllowlist(std::string_view comma_list) {
  std::size_t start = 0;
  while (start <= comma_list.size()) {
    const std::size_t comma = comma_list.find(',', start);
    const std::string_view entry =
        comma == std::string_view::npos
            ? comma_list.substr(start)
            : comma_list.substr(start, comma - start);
    const std::string domain = NormalizeDomain(entry);
    if (!domain.empty() && !IsValidDomainEntry(domain)) {
      return false;
    }
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }
  return true;
}

std::vector<std::string> ParseDomainAllowlist(std::string_view comma_list) {
  std::vector<std::string> allowed;
  std::size_t start = 0;
  while (start <= comma_list.size()) {
    const std::size_t comma = comma_list.find(',', start);
    const std::string_view entry =
        comma == std::string_view::npos
            ? comma_list.substr(start)
            : comma_list.substr(start, comma - start);
    const std::string domain = NormalizeDomain(entry);
    if (IsValidDomainEntry(domain) &&
        std::find(allowed.begin(), allowed.end(), domain) == allowed.end()) {
      allowed.push_back(domain);
    }
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }
  return allowed;
}

bool DomainAllowed(std::string_view host,
                   const std::vector<std::string>& allowed) {
  if (allowed.empty()) {
    return true;
  }
  std::string normalized;
  normalized.reserve(host.size());
  for (const char c : host) {
    normalized.push_back(LowerAscii(c));
  }
  while (!normalized.empty() && normalized.back() == '.') {
    normalized.pop_back();
  }
  for (const std::string& domain : allowed) {
    if (normalized == domain) {
      return true;
    }
    if (normalized.size() > domain.size() &&
        normalized.compare(normalized.size() - domain.size(), domain.size(),
                           domain) == 0 &&
        normalized[normalized.size() - domain.size() - 1] == '.') {
      return true;
    }
  }
  return false;
}

std::string JoinDomainAllowlist(const std::vector<std::string>& allowed) {
  std::string joined;
  for (const std::string& domain : allowed) {
    if (!joined.empty()) {
      joined.push_back(',');
    }
    joined += domain;
  }
  return joined;
}

}  // namespace runtime
}  // namespace mocktail

namespace mocktail {
namespace runtime {

namespace {

// ---- DNS wire helpers -------------------------------------------------

constexpr std::uint16_t kDnsTypeA = 1;
constexpr std::uint16_t kDnsTypePtr = 12;
constexpr std::uint16_t kDnsClassIn = 1;

void AppendName(std::vector<std::uint8_t>* out, const std::string& name) {
  std::size_t start = 0;
  while (start <= name.size()) {
    const std::size_t dot = name.find('.', start);
    const std::string label =
        dot == std::string::npos ? name.substr(start)
                                 : name.substr(start, dot - start);
    if (label.empty() || label.size() > 63) {
      out->push_back(0);
      return;
    }
    out->push_back(static_cast<std::uint8_t>(label.size()));
    out->insert(out->end(), label.begin(), label.end());
    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }
  out->push_back(0);
}

void AppendU16(std::vector<std::uint8_t>* out, std::uint16_t value) {
  out->push_back(static_cast<std::uint8_t>(value >> 8));
  out->push_back(static_cast<std::uint8_t>(value & 0xFF));
}

// Reads a (possibly compressed) domain name at `offset`. Returns nullopt on
// malformed data; advances `offset` past the name in the original stream.
std::optional<std::string> ReadDnsName(const std::uint8_t* data,
                                       std::size_t length,
                                       std::size_t* offset) {
  std::string name;
  std::size_t cursor = *offset;
  int jumps = 0;
  std::size_t next = 0;
  bool jumped = false;
  while (true) {
    if (cursor >= length) {
      return std::nullopt;
    }
    const std::uint8_t label_length = data[cursor];
    if (label_length == 0) {
      ++cursor;
      if (!jumped) {
        next = cursor;
      }
      break;
    }
    if ((label_length & 0xC0) == 0xC0) {
      if (cursor + 1 >= length) {
        return std::nullopt;
      }
      const std::size_t pointer =
          ((label_length & 0x3F) << 8) | data[cursor + 1];
      if (!jumped) {
        next = cursor + 2;
      }
      if (++jumps > 16 || pointer >= length) {
        return std::nullopt;
      }
      cursor = pointer;
      jumped = true;
      continue;
    }
    if ((label_length & 0xC0) != 0 || cursor + 1 + label_length > length) {
      return std::nullopt;
    }
    if (!name.empty()) {
      name.push_back('.');
    }
    name.append(reinterpret_cast<const char*>(data + cursor + 1), label_length);
    cursor += 1 + label_length;
  }
  *offset = jumped ? next : cursor;
  return name;
}

template <typename T, typename Handler>
std::vector<T> ParseAnswers(const std::uint8_t* data, std::size_t length,
                            std::uint16_t wanted_type, Handler handler) {
  std::vector<T> results;
  if (length < 12) {
    return results;
  }
  const std::uint16_t qd = (data[4] << 8) | data[5];
  const std::uint16_t an = (data[6] << 8) | data[7];
  std::size_t offset = 12;
  for (std::uint16_t i = 0; i < qd; ++i) {
    const auto name = ReadDnsName(data, length, &offset);
    if (!name.has_value() || offset + 4 > length) {
      return results;
    }
    offset += 4;  // qtype + qclass
  }
  for (std::uint16_t i = 0; i < an; ++i) {
    const auto name = ReadDnsName(data, length, &offset);
    if (!name.has_value() || offset + 10 > length) {
      return results;
    }
    const std::uint16_t type = (data[offset] << 8) | data[offset + 1];
    const std::uint16_t rdlength = (data[offset + 8] << 8) | data[offset + 9];
    offset += 10;
    if (offset + rdlength > length) {
      return results;
    }
    if (type == wanted_type) {
      handler(data, length, offset, rdlength, &results);
    }
    offset += rdlength;
  }
  return results;
}

}  // namespace

bool IsNumericIpv4(std::string_view host) {
  std::uint32_t address = 0;
  return ParseIpv4(host, &address);
}

bool ParseIpv4(std::string_view host, std::uint32_t* address) {
  if (address == nullptr) {
    return false;
  }
  struct in_addr parsed {};
  const std::string text(host);
  if (::inet_pton(AF_INET, text.c_str(), &parsed) != 1) {
    return false;
  }
  *address = ::ntohl(parsed.s_addr);
  return true;
}

std::string Ipv4ToPtrName(std::uint32_t address) {
  std::string name;
  for (int shift = 0; shift <= 24; shift += 8) {
    if (!name.empty()) {
      name.push_back('.');
    }
    name += std::to_string((address >> shift) & 0xFF);
  }
  name += ".in-addr.arpa";
  return name;
}

std::vector<std::uint8_t> BuildDnsQuery(std::uint16_t id,
                                        const std::string& qname,
                                        std::uint16_t qtype) {
  std::vector<std::uint8_t> packet;
  AppendU16(&packet, id);
  AppendU16(&packet, 0x0100);  // RD
  AppendU16(&packet, 1);       // QDCOUNT
  AppendU16(&packet, 0);
  AppendU16(&packet, 0);
  AppendU16(&packet, 0);
  AppendName(&packet, qname);
  AppendU16(&packet, qtype);
  AppendU16(&packet, kDnsClassIn);
  return packet;
}

std::vector<std::string> ParseDnsPtrAnswers(const std::uint8_t* data,
                                            std::size_t length) {
  return ParseAnswers<std::string>(
      data, length, kDnsTypePtr,
      [](const std::uint8_t* d, std::size_t len, std::size_t offset,
         std::uint16_t rdlength, std::vector<std::string>* out) {
        (void)rdlength;
        std::size_t cursor = offset;
        const auto name = ReadDnsName(d, len, &cursor);
        if (name.has_value() && !name->empty()) {
          out->push_back(*name);
        }
      });
}

std::vector<std::uint32_t> ParseDnsAAnswers(const std::uint8_t* data,
                                            std::size_t length) {
  return ParseAnswers<std::uint32_t>(
      data, length, kDnsTypeA,
      [](const std::uint8_t* d, std::size_t len, std::size_t offset,
         std::uint16_t rdlength, std::vector<std::uint32_t>* out) {
        (void)d;
        (void)len;
        if (rdlength == 4) {
          out->push_back((static_cast<std::uint32_t>(d[offset]) << 24) |
                         (static_cast<std::uint32_t>(d[offset + 1]) << 16) |
                         (static_cast<std::uint32_t>(d[offset + 2]) << 8) |
                         static_cast<std::uint32_t>(d[offset + 3]));
        }
      });
}

RawIpDecision DecideRawIp(const std::vector<std::string>& ptr_names,
                          const std::vector<std::uint32_t>& forward_addrs,
                          std::uint32_t attempted_addr,
                          const std::vector<std::string>& allowed) {
  if (allowed.empty()) {
    return RawIpDecision::kAllow;
  }
  for (const std::string& ptr : ptr_names) {
    if (!DomainAllowed(ptr, allowed)) {
      continue;
    }
    for (const std::uint32_t forward : forward_addrs) {
      if (forward == attempted_addr) {
        return RawIpDecision::kAllow;
      }
    }
  }
  return RawIpDecision::kBlock;
}

std::vector<std::string> DefaultPtrResolvers() {
  const char* override_resolvers = std::getenv("MOCKTAIL_PTR_RESOLVERS");
  if (override_resolvers != nullptr && override_resolvers[0] != '\0') {
    std::vector<std::string> parsed;
    std::size_t start = 0;
    const std::string raw(override_resolvers);
    while (start <= raw.size()) {
      const std::size_t comma = raw.find(',', start);
      const std::string entry =
          comma == std::string::npos ? raw.substr(start)
                                     : raw.substr(start, comma - start);
      std::uint32_t ignored = 0;
      if (ParseIpv4(entry, &ignored)) {
        parsed.push_back(entry);
      }
      if (comma == std::string::npos) {
        break;
      }
      start = comma + 1;
    }
    if (!parsed.empty()) {
      return parsed;
    }
  }
  return {"8.8.8.8", "8.8.4.4", "1.1.1.1"};
}

namespace {

std::optional<std::vector<std::uint8_t>> UdpQuery(
    const std::string& resolver, std::uint16_t port,
    const std::vector<std::uint8_t>& query, int timeout_ms) {
  std::uint32_t resolver_addr = 0;
  if (!ParseIpv4(resolver, &resolver_addr)) {
    return std::nullopt;
  }
  const int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    return std::nullopt;
  }
  struct timeval tv {};
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  struct sockaddr_in destination {};
  destination.sin_family = AF_INET;
  destination.sin_port = ::htons(port);
  destination.sin_addr.s_addr = ::htonl(resolver_addr);
  if (::sendto(fd, query.data(), query.size(), 0,
               reinterpret_cast<const struct sockaddr*>(&destination),
               sizeof(destination)) < 0) {
    ::close(fd);
    return std::nullopt;
  }
  std::array<std::uint8_t, 4096> buffer {};
  const ssize_t received =
      ::recv(fd, buffer.data(), buffer.size(), 0);
  ::close(fd);
  if (received <= 12) {
    return std::nullopt;
  }
  return std::vector<std::uint8_t>(buffer.data(),
                                   buffer.data() + received);
}

template <typename T>
std::optional<std::vector<T>> QueryRecords(
    const std::string& qname, std::uint16_t qtype,
    const std::vector<std::string>& resolvers, int timeout_ms,
    std::vector<T> (*parse)(const std::uint8_t*, std::size_t)) {
  static std::uint16_t query_id = 0x4D54;
  for (const std::string& resolver : resolvers) {
    const std::vector<std::uint8_t> query =
        BuildDnsQuery(++query_id, qname, qtype);
    const auto response = UdpQuery(resolver, 53, query, timeout_ms);
    if (!response.has_value()) {
      continue;
    }
    // Ignore responses with the TC bit or an error rcode.
    if (response->size() < 12 || (response->at(2) & 0x02) != 0 ||
        (response->at(3) & 0x0F) != 0) {
      continue;
    }
    std::vector<T> answers = parse(response->data(), response->size());
    if (!answers.empty()) {
      return answers;
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<std::vector<std::string>> QueryPtrRecords(
    std::uint32_t address, const std::vector<std::string>& resolvers,
    int timeout_ms) {
  return QueryRecords<std::string>(Ipv4ToPtrName(address), kDnsTypePtr,
                                   resolvers, timeout_ms, ParseDnsPtrAnswers);
}

std::optional<std::vector<std::uint32_t>> QueryARecords(
    const std::string& name, const std::vector<std::string>& resolvers,
    int timeout_ms) {
  return QueryRecords<std::uint32_t>(name, kDnsTypeA, resolvers, timeout_ms,
                                     ParseDnsAAnswers);
}

bool RawIpv4Allowed(std::uint32_t address,
                    const std::vector<std::string>& allowed) {
  if (allowed.empty()) {
    return true;
  }
  static std::mutex cache_mutex;
  static std::unordered_map<std::uint32_t, bool> cache;
  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    const auto found = cache.find(address);
    if (found != cache.end()) {
      return found->second;
    }
  }
  const std::vector<std::string> resolvers = DefaultPtrResolvers();
  bool allowed_result = false;
  const auto ptr_names = QueryPtrRecords(address, resolvers, 1500);
  if (ptr_names.has_value()) {
    std::vector<std::uint32_t> forward;
    for (const std::string& name : *ptr_names) {
      if (!DomainAllowed(name, allowed)) {
        continue;
      }
      const auto addrs = QueryARecords(name, resolvers, 1500);
      if (addrs.has_value()) {
        forward.insert(forward.end(), addrs->begin(), addrs->end());
      }
    }
    allowed_result = DecideRawIp(*ptr_names, forward, address, allowed) ==
                     RawIpDecision::kAllow;
  }
  std::lock_guard<std::mutex> lock(cache_mutex);
  cache[address] = allowed_result;
  return allowed_result;
}

}  // namespace runtime
}  // namespace mocktail
