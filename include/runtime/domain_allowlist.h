#ifndef MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_
#define MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mocktail {
namespace runtime {

// Parses a comma-separated domain allowlist (e.g. "roblox.com,google.com").
// Entries are trimmed, lowercased, and stripped of a leading "*." and a
// trailing "."; empty and malformed entries are dropped. Returns the
// normalized list. An empty result means "no restriction".
std::vector<std::string> ParseDomainAllowlist(std::string_view comma_list);

// True when `entry` is a syntactically valid domain name (letters, digits,
// hyphens, dots; labels must not be empty or start/end with a hyphen).
bool IsValidDomainEntry(std::string_view entry);

// True when every comma-separated entry is blank or a valid domain, i.e. the
// list can be enforced as-is.
bool IsValidDomainAllowlist(std::string_view comma_list);

// True when `host` equals an allowed domain or is a subdomain of one
// (case-insensitive; a trailing dot on the host is ignored). An empty
// allowlist allows everything.
bool DomainAllowed(std::string_view host,
                   const std::vector<std::string>& allowed);

// Joins a normalized allowlist back into a comma-separated string (used to
// export the policy through the environment).
std::string JoinDomainAllowlist(const std::vector<std::string>& allowed);

// True when `host` is a dotted-quad IPv4 literal (e.g. "203.0.113.7").
bool IsNumericIpv4(std::string_view host);

// Parses a dotted-quad IPv4 literal into a host-order address. Returns false
// when `host` is not a numeric IPv4 literal.
bool ParseIpv4(std::string_view host, std::uint32_t* address);

// "203.0.113.7" -> "7.113.0.203.in-addr.arpa".
std::string Ipv4ToPtrName(std::uint32_t address);

// Builds a DNS query packet for `qname`/`qtype` (QCLASS IN).
std::vector<std::uint8_t> BuildDnsQuery(std::uint16_t id,
                                        const std::string& qname,
                                        std::uint16_t qtype);

// Parses the PTR (type 12) answers of a DNS response packet. Returns the
// target names (decompressed). Malformed packets yield an empty list.
std::vector<std::string> ParseDnsPtrAnswers(const std::uint8_t* data,
                                            std::size_t length);

// Parses the A (type 1) answers of a DNS response packet into host-order
// IPv4 addresses.
std::vector<std::uint32_t> ParseDnsAAnswers(const std::uint8_t* data,
                                            std::size_t length);

// Decision for a raw-IP connection attempt: PTR names that pass the domain
// allowlist are only honored when the forward A records of that name point
// back to the attempted address (anti-spoofing: an attacker who controls
// the reverse zone of their IP must not be able to claim "roblox.com").
enum class RawIpDecision { kBlock, kAllow };
RawIpDecision DecideRawIp(const std::vector<std::string>& ptr_names,
                          const std::vector<std::uint32_t>& forward_addrs,
                          std::uint32_t attempted_addr,
                          const std::vector<std::string>& allowed);

// Default public resolvers used for raw-IP reverse/forward lookups.
std::vector<std::string> DefaultPtrResolvers();

// Resolves the PTR names of `address` by querying the given resolvers over
// UDP/53 (tried in order, `timeout_ms` each). Network I/O; returns nullopt
// when no resolver answered.
std::optional<std::vector<std::string>> QueryPtrRecords(
    std::uint32_t address, const std::vector<std::string>& resolvers,
    int timeout_ms);

// Resolves the A records of `name` via the given resolvers. Network I/O.
std::optional<std::vector<std::uint32_t>> QueryARecords(
    const std::string& name, const std::vector<std::string>& resolvers,
    int timeout_ms);

// Full raw-IP gate: reverse-resolve `address` via `resolvers`, check the PTR
// names against `allowed`, forward-verify, and decide. Results are cached
// per address for the process lifetime. An empty `allowed` allows
// everything without any network I/O.
bool RawIpv4Allowed(std::uint32_t address,
                    const std::vector<std::string>& allowed);

}  // namespace runtime
}  // namespace mocktail

#endif  // MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_
