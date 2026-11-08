#include "runtime/domain_allowlist.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using mocktail::runtime::DomainAllowed;
using mocktail::runtime::IsValidDomainAllowlist;
using mocktail::runtime::IsValidDomainEntry;
using mocktail::runtime::JoinDomainAllowlist;
using mocktail::runtime::ParseDomainAllowlist;

namespace {

TEST(DomainAllowlistTest, ParsesCommaSeparatedDomains) {
  const auto parsed =
      ParseDomainAllowlist("roblox.com, google.com ,CLOUDFLARE.COM.");
  ASSERT_EQ(parsed.size(), 3U);
  EXPECT_EQ(parsed[0], "roblox.com");
  EXPECT_EQ(parsed[1], "google.com");
  EXPECT_EQ(parsed[2], "cloudflare.com");
  EXPECT_EQ(JoinDomainAllowlist(parsed),
            "roblox.com,google.com,cloudflare.com");
}

TEST(DomainAllowlistTest, DropsEmptyInvalidAndDuplicateEntries) {
  const auto parsed =
      ParseDomainAllowlist("roblox.com,,bad_domain!,roblox.com,*.google.com");
  ASSERT_EQ(parsed.size(), 2U);
  EXPECT_EQ(parsed[0], "roblox.com");
  EXPECT_EQ(parsed[1], "google.com");
  EXPECT_TRUE(ParseDomainAllowlist("").empty());
  EXPECT_TRUE(ParseDomainAllowlist(" , ,").empty());
}

TEST(DomainAllowlistTest, ValidatesEntries) {
  EXPECT_TRUE(IsValidDomainEntry("roblox.com"));
  EXPECT_TRUE(IsValidDomainEntry("www.roblox.com"));
  EXPECT_TRUE(IsValidDomainEntry("my-host.example.co.uk"));
  EXPECT_TRUE(IsValidDomainEntry("*.ROBLOX.com."));
  EXPECT_FALSE(IsValidDomainEntry(""));
  EXPECT_FALSE(IsValidDomainEntry("bad_domain"));
  EXPECT_FALSE(IsValidDomainEntry("-lead.com"));
  EXPECT_FALSE(IsValidDomainEntry("trail-.com"));
  EXPECT_FALSE(IsValidDomainEntry("double..dot.com"));
  EXPECT_FALSE(IsValidDomainEntry("has space.com"));
  EXPECT_TRUE(IsValidDomainAllowlist("roblox.com,google.com"));
  EXPECT_TRUE(IsValidDomainAllowlist(""));
  EXPECT_FALSE(IsValidDomainAllowlist("roblox.com,bad_domain"));
}

TEST(DomainAllowlistTest, EmptyListAllowsEverything) {
  EXPECT_TRUE(DomainAllowed("anything.example", {}));
  EXPECT_TRUE(DomainAllowed("", {}));
}

TEST(DomainAllowlistTest, MatchesDomainAndSubdomainsOnly) {
  const auto allowed = ParseDomainAllowlist("roblox.com,google.com");
  EXPECT_TRUE(DomainAllowed("roblox.com", allowed));
  EXPECT_TRUE(DomainAllowed("www.roblox.com", allowed));
  EXPECT_TRUE(DomainAllowed("deep.sub.roblox.com", allowed));
  EXPECT_TRUE(DomainAllowed("ROBLOX.COM", allowed));
  EXPECT_TRUE(DomainAllowed("WWW.Roblox.Com.", allowed));
  EXPECT_TRUE(DomainAllowed("google.com", allowed));
  EXPECT_TRUE(DomainAllowed("accounts.google.com", allowed));
  // Not the domain itself, and not a suffix-embedded lookalike.
  EXPECT_FALSE(DomainAllowed("evilroblox.com", allowed));
  EXPECT_FALSE(DomainAllowed("roblox.com.evil.com", allowed));
  EXPECT_FALSE(DomainAllowed("notroblox.com", allowed));
  EXPECT_FALSE(DomainAllowed("discord.com", allowed));
  EXPECT_FALSE(DomainAllowed("1.2.3.4", allowed));
  EXPECT_FALSE(DomainAllowed("", allowed));
}

}  // namespace
