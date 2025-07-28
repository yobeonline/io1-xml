#pragma once

#include <array>
#include <type_traits>
#include <string_view>
#include <iostream>
#include <cassert>
#include <utility>

namespace io1::xml
{
  template<typename T>
  struct attr
  {
    explicit attr(std::string_view name, T const &value) noexcept:name(name), value(value) {}
    std::string_view name;
    T const &value;
  };

  struct tag
  {
    explicit tag(std::string_view name) noexcept:name(name) {}
    std::string_view name;
  };

  struct tree
  {
    explicit tree(std::string_view name) noexcept:name(name) {}
    std::string_view name;
  };

  namespace details
  {
    template<char indent_char, unsigned indent_increment, unsigned indent_size>
    struct indentation
    {
      using increased_t = indentation<indent_char, indent_increment, indent_size+indent_increment>;

      friend std::ostream & operator<<(std::ostream & stream, const indentation &) noexcept
      {
        constexpr static auto const indent_array = []
        {
          std::array<char, indent_size + 1> values;

          for (auto & c : values) c = indent_char;
          values[indent_size] = '\0';

          return values;
        }();

        return stream << indent_array.data();
      }
   
    };

    struct done {};

    template <class indent>
    struct tree_impl;
      
    template <class indent, bool tree_tag = false, char closing = '/'>
    struct tag_impl
    {
      explicit tag_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name)
      {
        stream_ << indent_ << '<' << name_;
      }

      tag_impl(tag_impl && other) noexcept : stream_(other.stream_), name_(other.name_)
      {
        other.active_ = false;
      }

      ~tag_impl() noexcept
      {
        if (!active_) return;
        if (empty_)
        {
          stream_ << ' ' << closing << ">\n";
        }
        else
        {
          if constexpr (tree_tag)
          {
            stream_ << indent_ << '<' << closing << name_ << ">\n";
          }
          else
          {
            stream_ << '<' << closing << name_ << ">\n";
          }
        }
      }

      template<typename T> tag_impl & operator<<(attr<T> const & a) noexcept
      {
        stream_ << ' ' << a.name << "=\"" << a.value << '\"';
        return *this;
      }

      tree_impl<typename indent::increased_t>
      operator<<(tree const & t) noexcept
      {
        if (empty_)
        {
          stream_ << ">\n";
          empty_ = false;
        }

        return tree_impl<typename indent::increased_t>{stream_, t.name};
      }
        
      tag_impl<typename indent::increased_t> operator<<(tag const & t) noexcept
      {
        if (empty_)
        {
          stream_ << ">\n";
          empty_ = false;
        }

        return tag_impl<typename indent::increased_t>{stream_, t.name};
      }

      template <typename T>
      done operator<<(T const & value) noexcept
      {
        stream_ << '>' << value;
        empty_ = false;

        return {};
      }

      bool active_{true};
      bool empty_{true};
      std::ostream & stream_;
      std::string_view name_;
      constexpr static indent indent_;
    };

    template<class indent>
    struct tree_impl
    {
      explicit tree_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name), tag_(stream, name)
      {}

      ~tree_impl() noexcept =default;

      tree_impl(tree_impl && t) noexcept =default;

      template<typename T> tree_impl && operator<<(attr<T> const & a) && noexcept
      {
        tag_ << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept
      {
        return tag_ << t;
      }

      auto operator<<(tree const & t) & noexcept
      {
        return tag_ << t;
      }

      std::ostream & stream_;
      std::string_view name_;
      tag_impl<indent, true> tag_; // it is a special tag in the way that it closes on a new line
    };

    template<class indent> struct prolog_impl
    {
      explicit prolog_impl(std::ostream & stream, std::string_view encoding, bool standalone) noexcept
      {
        tag_impl<indent, false, '?'>(stream, "?xml")
          << attr("version", "1.0")
          << attr("encoding", encoding)
          << attr("standalone", (standalone ? "yes" : "no"));
      }

      prolog_impl(prolog_impl &&) noexcept {}
    };

    template<char indent_char=' ', unsigned indent_increment=2>
    struct doc_impl
    {
      explicit doc_impl(std::ostream & stream,
        std::string_view root_name,
        std::string_view encoding = "UTF-8",
        bool standalone=true
      ) noexcept:
      prolog_(stream, encoding, standalone),
      root_(stream, root_name)
      {}

      doc_impl(doc_impl && d) noexcept:
        prolog_(std::move(d.prolog_)),
        root_(std::move(d.root_))
      {}

      template<typename T> doc_impl && operator<<(attr<T> const & a) && noexcept
      {
        std::move(root_) << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept
      {
        return root_ << t;
      }

      auto operator<<(tree const & t) & noexcept
      {
        return root_ << t;
      }

      prolog_impl<indentation<indent_char, indent_increment, 0>> prolog_;
      tree_impl<indentation<indent_char, indent_increment, 0>> root_;
    };
  }

  template<char indent_char=' ', unsigned indent_inc=2> using doc = details::doc_impl<indent_char,indent_inc>;
}