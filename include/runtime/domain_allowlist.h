#ifndef MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_
#define MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_

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

}  // namespace runtime
}  // namespace mocktail

#endif  // MOCKTAIL_RUNTIME_DOMAIN_ALLOWLIST_H_
