#include "runtime/domain_allowlist.h"

#include <algorithm>
#include <cctype>

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
