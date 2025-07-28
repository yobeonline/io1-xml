#include <iostream>
#include "io1/xml.hpp"

int main()
{
  using namespace io1::xml;
  auto d = doc(std::cout, "bookstore");
    
  {
      auto book = d << tree("book") << attr("category", "fiction") << attr("lang", "en");
      book << tag("title") << "The Time Machine";
      book << tag("author") << "H. G. Wells";
      book << tag("price") << attr("currency", "USD") << 15.99;
  }

  {
      auto book = d << tree("book") << attr("category", "history") << attr("lang", "fr");
      book << tag("title") << "Les Miserables";
      book << tag("author") << "Victor & Hugo";
      book << tag("price") << attr("currency", "EUR") << 12.50;
  }

  {
      d << tree("magazine") << attr("title", "Science Weekly");
  }

  {
      auto newspaper = d << tree("newspaper") << attr("date", "2025-07-14");
      newspaper << tag("headline") << "AI Advances in Compile-Time XML";
  }

  return 0;
}
