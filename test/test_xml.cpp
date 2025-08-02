#include "io1/xml.hpp" // Replace with actual header name

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

using namespace io1::xml;

auto getlines(std::istream& stream) noexcept
{
  std::vector<std::string> lines;

  for (std::string line; std::getline(stream, line); )
  {
    lines.push_back(std::move(line));
  }

  return lines;
}

TEST_CASE("Empty default xml")
{
  std::stringstream ss;
  doc(ss, "root");

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", false);

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

// A helper to compare XML outputs
std::string normalize(std::string const & s)
{
  std::string r;
  for (char c : s)
    if (c != '\r') r += c;
  return r;
}

TEST_CASE("Basic XML generation")
{
  std::ostringstream oss;
  {
    auto d = doc<' ', 2>(oss, "root") << attr("lang", "en");
    d << tag("child") << attr("id", 42) << "content";
    }

  CHECK(normalize(oss.str()) ==
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root lang="en">
  <child id="42">content</child>
</root>
)");
}

TEST_CASE("Nested trees")
{
  std::ostringstream oss;
  {
    doc<' ', 2> d(oss, "root");
    auto t = d << tree("level1");
    auto t2 = t << tree("level2");
    auto leaf = t2 << tag("leaf");
    leaf << attr("value", "abc") << "text";
  }

  CHECK(oss.str().find("<level1>") != std::string::npos);
  CHECK(oss.str().find("<leaf value=\"abc\">text</leaf>") != std::string::npos);
}

TEST_CASE("Special characters are escaped")
{
  std::ostringstream oss;
  {
    doc d(oss, "root");
    auto child = d << tag("child");
    child << std::string{"<hello & goodbye>"};
  }

  CHECK(oss.str().find("&lt;hello &amp; goodbye&gt;") != std::string::npos);
}