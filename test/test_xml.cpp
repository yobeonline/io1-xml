#include "io1/xml.hpp" // Replace with actual header name

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>
#include <vector>

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
  doc(ss, "root", config{.standalone=false});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not utf-8 xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="latin-9"});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="latin-9" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not utf-8 and not standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="latin-9", .standalone=false});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="latin-9" standalone="no" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty utf-8 and standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="UTF-8", .standalone=true});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty with attributes")
{
  std::stringstream ss;
  doc(ss, "root") << attr("lang", "en");

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root lang="en" />)");
}

TEST_CASE("Attributes escaping: quotes, less-than and greater-than") {
  std::stringstream ss;
  doc(ss, "root") << attr("v", R"("a&b<c>d")");
  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[1] == R"(<root v="&quot;a&amp;b&lt;c&gt;d&quot;" />)");
}

TEST_CASE("Multiple attributes with escaping") {
  std::stringstream ss;
  doc(ss, "root") << attr("a", "1&2") << attr("b", "x<y");
  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  // If attribute order is defined as insertion-order:
  CHECK(lines[1] == R"(<root a="1&amp;2" b="x&lt;y" />)");
}

TEST_CASE("Root with single child tag")
{
  std::stringstream ss;
  {
    auto d = doc(ss, "root");
    d << tag("foo");
  }

  auto const lines = getlines(ss);
  CHECK(lines.size() == 4);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root>)");
  CHECK(lines[2] == R"(  <foo />)");
  CHECK(lines[3] == R"(</root>)");
}

TEST_CASE("Root with two child tags preserves order") {
  std::stringstream ss;
  {
    auto d = doc(ss, "root");
    d << tag("foo");
    d << tag("bar");
  }
  auto const lines = getlines(ss);
  CHECK(lines.size() == 5);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root>)");
  CHECK(lines[2] == R"(  <foo />)");
  CHECK(lines[3] == R"(  <bar />)");
  CHECK(lines[4] == R"(</root>)");
}