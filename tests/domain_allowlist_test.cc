#include "runtime/domain_allowlist.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>

using mocktail::runtime::BuildDnsQuery;
using mocktail::runtime::DecideRawIp;
using mocktail::runtime::DefaultPtrResolvers;
using mocktail::runtime::DomainAllowed;
using mocktail::runtime::Ipv4ToPtrName;
using mocktail::runtime::IsNumericIpv4;
using mocktail::runtime::IsValidDomainAllowlist;
using mocktail::runtime::IsValidDomainEntry;
using mocktail::runtime::IsValidPtrResolverList;
using mocktail::runtime::JoinDomainAllowlist;
using mocktail::runtime::JoinPtrResolvers;
using mocktail::runtime::ParseDnsAAnswers;
using mocktail::runtime::ParseDnsPtrAnswers;
using mocktail::runtime::ParseDomainAllowlist;
using mocktail::runtime::ParseIpv4;
using mocktail::runtime::ParsePtrResolverList;
using mocktail::runtime::RawIpDecision;
using mocktail::runtime::RawIpDomainCache;

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

TEST(DomainAllowlistTest, DetectsNumericIpv4Literals) {
  EXPECT_TRUE(IsNumericIpv4("203.0.113.7"));
  EXPECT_TRUE(IsNumericIpv4("8.8.8.8"));
  EXPECT_FALSE(IsNumericIpv4("roblox.com"));
  EXPECT_FALSE(IsNumericIpv4("203.0.113.256"));
  EXPECT_FALSE(IsNumericIpv4("203.0.113"));
  EXPECT_FALSE(IsNumericIpv4(""));
  std::uint32_t address = 0;
  ASSERT_TRUE(ParseIpv4("203.0.113.7", &address));
  EXPECT_EQ(address, 0xCB007107U);
  EXPECT_FALSE(ParseIpv4("example.com", &address));
  EXPECT_EQ(Ipv4ToPtrName(0xCB007107U), "7.113.0.203.in-addr.arpa");
}

TEST(DomainAllowlistTest, BuildsDnsQueryPacket) {
  const auto packet = BuildDnsQuery(0x1234, "7.113.0.203.in-addr.arpa", 12);
  ASSERT_GE(packet.size(), 12U + 23U + 4U);
  EXPECT_EQ(packet[0], 0x12);
  EXPECT_EQ(packet[1], 0x34);
  EXPECT_EQ(packet[2] << 8 | packet[3], 0x0100);  // RD
  EXPECT_EQ(packet[4] << 8 | packet[5], 1);       // QDCOUNT
  // QNAME labels: 1 '7', 3 '113', 1 '0', 3 '203', 7 'in-addr', 4 'arpa', 0
  EXPECT_EQ(packet[12], 1);
  EXPECT_EQ(packet[13], '7');
  ASSERT_GE(packet.size(), 4U);
  EXPECT_EQ(packet[packet.size() - 4] << 8 | packet[packet.size() - 3],
            12);                                     // QTYPE PTR
  EXPECT_EQ(packet[packet.size() - 2] << 8 | packet[packet.size() - 1],
            1);                                      // QCLASS IN
}

namespace {

void AppendU16(std::vector<std::uint8_t>* out, std::uint16_t v) {
  out->push_back(static_cast<std::uint8_t>(v >> 8));
  out->push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void AppendName(std::vector<std::uint8_t>* out, const std::string& name) {
  std::size_t start = 0;
  while (start <= name.size()) {
    const std::size_t dot = name.find('.', start);
    const std::string label = dot == std::string::npos
                                  ? name.substr(start)
                                  : name.substr(start, dot - start);
    out->push_back(static_cast<std::uint8_t>(label.size()));
    out->insert(out->end(), label.begin(), label.end());
    if (dot == std::string::npos) break;
    start = dot + 1;
  }
  out->push_back(0);
}

// A DNS response: one question (the PTR name), one PTR answer whose RDATA
// uses a compression pointer to a name embedded later in the packet.
std::vector<std::uint8_t> CraftPtrResponse() {
  std::vector<std::uint8_t> p;
  AppendU16(&p, 0x1234);   // id
  AppendU16(&p, 0x8180);   // QR + RD + RA, rcode 0
  AppendU16(&p, 1);        // QD
  AppendU16(&p, 1);        // AN
  AppendU16(&p, 0);
  AppendU16(&p, 0);
  AppendName(&p, "7.113.0.203.in-addr.arpa");  // question name at offset 12
  AppendU16(&p, 12);       // QTYPE PTR
  AppendU16(&p, 1);        // QCLASS IN
  p.push_back(0xC0);       // answer name: pointer to offset 12
  p.push_back(0x0C);
  AppendU16(&p, 12);       // TYPE PTR
  AppendU16(&p, 1);        // CLASS IN
  AppendU16(&p, 0);        // TTL
  AppendU16(&p, 60);
  const std::size_t rdlength_at = p.size();
  AppendU16(&p, 0);        // placeholder rdlength
  const std::size_t rdata_at = p.size();
  // RDATA: "edge" + compression pointer to "roblox.com" appended after.
  p.push_back(4);
  p.insert(p.end(), {'e', 'd', 'g', 'e'});
  const std::size_t tail_at = p.size() + 2;
  p.push_back(0xC0);
  p.push_back(static_cast<std::uint8_t>(tail_at));  // packet is < 256 bytes
  AppendName(&p, "roblox.com");
  const std::uint16_t rdlength =
      static_cast<std::uint16_t>(p.size() - rdata_at);
  p[rdlength_at] = static_cast<std::uint8_t>(rdlength >> 8);
  p[rdlength_at + 1] = static_cast<std::uint8_t>(rdlength & 0xFF);
  return p;
}

std::vector<std::uint8_t> CraftAResponse(std::uint32_t address) {
  std::vector<std::uint8_t> p;
  AppendU16(&p, 0x1234);
  AppendU16(&p, 0x8180);
  AppendU16(&p, 1);
  AppendU16(&p, 1);
  AppendU16(&p, 0);
  AppendU16(&p, 0);
  AppendName(&p, "edge.roblox.com");
  AppendU16(&p, 1);
  AppendU16(&p, 1);
  p.push_back(0xC0);
  p.push_back(0x0C);
  AppendU16(&p, 1);        // TYPE A
  AppendU16(&p, 1);        // CLASS IN
  AppendU16(&p, 0);
  AppendU16(&p, 60);
  AppendU16(&p, 4);        // RDLENGTH
  p.push_back(static_cast<std::uint8_t>(address >> 24));
  p.push_back(static_cast<std::uint8_t>(address >> 16));
  p.push_back(static_cast<std::uint8_t>(address >> 8));
  p.push_back(static_cast<std::uint8_t>(address));
  return p;
}

namespace {

struct TempCacheFile {
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("mocktail-da-cache-test-" + std::to_string(::getpid()) + "-" +
       std::to_string(counter++) + ".json");
  ~TempCacheFile() { std::error_code ec; std::filesystem::remove(path, ec); }
  static int counter;
};
int TempCacheFile::counter = 0;

}  // namespace

TEST(DomainAllowlistTest, CachePersistsAcrossInstances) {
  TempCacheFile temp;
  {
    RawIpDomainCache cache(temp.path, "version-1234");
    cache.Put(0xCB007107U, true);
    cache.Put(0x01010101U, false);
  }
  RawIpDomainCache reloaded(temp.path, "version-1234");
  bool allowed = false;
  ASSERT_TRUE(reloaded.Get(0xCB007107U, &allowed));
  EXPECT_TRUE(allowed);
  ASSERT_TRUE(reloaded.Get(0x01010101U, &allowed));
  EXPECT_FALSE(allowed);
  EXPECT_FALSE(reloaded.Get(0x08080808U, &allowed));
  // File is private.
  struct stat metadata {};
  ASSERT_EQ(::stat(temp.path.c_str(), &metadata), 0);
  EXPECT_EQ(metadata.st_mode & 0777, 0600);
}

TEST(DomainAllowlistTest, CacheDiscardedAfterPayloadVersionChange) {
  TempCacheFile temp;
  {
    RawIpDomainCache cache(temp.path, "version-old");
    cache.Put(0xCB007107U, true);
  }
  // Simulates a Roblox update: same file, new payload version.
  RawIpDomainCache after_update(temp.path, "version-new");
  bool allowed = true;
  EXPECT_FALSE(after_update.Get(0xCB007107U, &allowed));
  // And the next decision overwrites the file under the new version.
  after_update.Put(0xCB007107U, false);
  RawIpDomainCache verify(temp.path, "version-new");
  ASSERT_TRUE(verify.Get(0xCB007107U, &allowed));
  EXPECT_FALSE(allowed);
  RawIpDomainCache stale(temp.path, "version-old");
  EXPECT_FALSE(stale.Get(0xCB007107U, &allowed));
}

TEST(DomainAllowlistTest, CacheIgnoresMalformedFile) {
  TempCacheFile temp;
  {
    std::ofstream output(temp.path, std::ios::binary | std::ios::trunc);
    output << "not json at all {{{";
  }
  RawIpDomainCache cache(temp.path, "version-1");
  bool allowed = true;
  EXPECT_FALSE(cache.Get(0xCB007107U, &allowed));
  cache.Put(0xCB007107U, true);
  RawIpDomainCache reloaded(temp.path, "version-1");
  ASSERT_TRUE(reloaded.Get(0xCB007107U, &allowed));
  EXPECT_TRUE(allowed);
}

TEST(DomainAllowlistTest, EmptyAllowlistNeedsNoCacheOrNetwork) {
  TempCacheFile temp;
  RawIpDomainCache cache(temp.path, "version-1");
  EXPECT_TRUE(cache.Allowed(0xCB007107U, {}));
  EXPECT_FALSE(std::filesystem::exists(temp.path));
}

}  // namespace

TEST(DomainAllowlistTest, ParsesPtrAndAAnswersWithCompression) {
  const auto ptr = CraftPtrResponse();
  const auto names = ParseDnsPtrAnswers(ptr.data(), ptr.size());
  ASSERT_EQ(names.size(), 1U);
  EXPECT_EQ(names[0], "edge.roblox.com");
  const auto a = CraftAResponse(0xCB007107U);
  const auto addrs = ParseDnsAAnswers(a.data(), a.size());
  ASSERT_EQ(addrs.size(), 1U);
  EXPECT_EQ(addrs[0], 0xCB007107U);
  // Garbage packets yield nothing instead of crashing.
  EXPECT_TRUE(ParseDnsPtrAnswers(nullptr, 0).empty());
  const std::uint8_t junk[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xFF};
  EXPECT_TRUE(ParseDnsPtrAnswers(junk, sizeof(junk)).empty());
}

TEST(DomainAllowlistTest, DecideRawIpRequiresAllowlistedPtrAndForwardMatch) {
  const auto allowed = ParseDomainAllowlist("roblox.com");
  const std::uint32_t addr = 0xCB007107U;
  // PTR passes and forwards back to the attempted address: allow.
  EXPECT_EQ(DecideRawIp({"edge.roblox.com"}, {addr}, addr, allowed),
            RawIpDecision::kAllow);
  // PTR passes but forwards elsewhere: block (spoof attempt).
  EXPECT_EQ(DecideRawIp({"edge.roblox.com"}, {0x01010101U}, addr, allowed),
            RawIpDecision::kBlock);
  // PTR not allowlisted: block.
  EXPECT_EQ(DecideRawIp({"ptr.evil.com"}, {addr}, addr, allowed),
            RawIpDecision::kBlock);
  // No PTR at all: block.
  EXPECT_EQ(DecideRawIp({}, {addr}, addr, allowed), RawIpDecision::kBlock);
  // Empty allowlist: allow without any lookups.
  EXPECT_EQ(DecideRawIp({}, {}, addr, {}), RawIpDecision::kAllow);
}

TEST(DomainAllowlistTest, ParsesAndValidatesPtrResolverLists) {
  const auto parsed = ParsePtrResolverList(" 8.8.8.8 , 1.1.1.1 ,garbage,,8.8.8.8");
  ASSERT_EQ(parsed.size(), 2U);
  EXPECT_EQ(parsed[0], "8.8.8.8");
  EXPECT_EQ(parsed[1], "1.1.1.1");
  EXPECT_EQ(JoinPtrResolvers(parsed), "8.8.8.8,1.1.1.1");
  EXPECT_TRUE(IsValidPtrResolverList("8.8.8.8,8.8.4.4,1.1.1.1"));
  EXPECT_TRUE(IsValidPtrResolverList(" 9.9.9.9 "));
  EXPECT_FALSE(IsValidPtrResolverList(""));
  EXPECT_FALSE(IsValidPtrResolverList("8.8.8.8,not-an-ip"));
  EXPECT_FALSE(IsValidPtrResolverList("999.1.1.1"));
  EXPECT_FALSE(IsValidPtrResolverList("8.8.8.8,"));
  EXPECT_TRUE(ParsePtrResolverList("").empty());
}

TEST(DomainAllowlistTest, DefaultPtrResolversHonorsEnvOverride) {
  unsetenv("MOCKTAIL_PTR_RESOLVERS");
  EXPECT_EQ(DefaultPtrResolvers(),
            (std::vector<std::string>{"8.8.8.8", "8.8.4.4", "1.1.1.1"}));
  setenv("MOCKTAIL_PTR_RESOLVERS", "9.9.9.9, 149.112.112.112", 1);
  EXPECT_EQ(DefaultPtrResolvers(),
            (std::vector<std::string>{"9.9.9.9", "149.112.112.112"}));
  // An all-invalid override falls back to the defaults.
  setenv("MOCKTAIL_PTR_RESOLVERS", "not-an-ip", 1);
  EXPECT_EQ(DefaultPtrResolvers(),
            (std::vector<std::string>{"8.8.8.8", "8.8.4.4", "1.1.1.1"}));
  unsetenv("MOCKTAIL_PTR_RESOLVERS");
}

}  // namespace
