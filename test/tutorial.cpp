#include <iostream>
#include "io1/xml.hpp"

int main()
{
    auto doc = io1::xml_doc(std::cout, "bookstore");
    
    {
        auto book = doc.tree("book") << io1::attr("category", "fiction") << io1::attr("lang", "en");
        book << io1::tag("title") << "The Time Machine";
        book << io1::tag("author") << "H. G. Wells";
        book << io1::tag("price") << io1::attr("currency", "USD") << 15.99;
    }

    {
        auto book = doc.tree("book") << io1::attr("category", "history") << io1::attr("lang", "fr");
        book << io1::tag("title") << "Les Misérables";
        book << io1::tag("author") << "Victor Hugo";
        book << io1::tag("price") << io1::attr("currency", "EUR") << 12.50;
    }

    {
        doc.tree("magazine") << io1::attr("title", "Science Weekly");
    }

    {
        auto newspaper = doc.tree("newspaper") << io1::attr("date", "2025-07-14");
        newspaper << io1::tag("headline") << "AI Advances in Compile-Time XML";
    }

    return 0;
}
