/*
 * This file is part of the hrewrite library.
 * Copyright (c) 2021 ONERA.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 // Author: Michael Lienhardt
 // Maintainer: Michael Lienhardt
 // email: michael.lienhardt@onera.fr

#ifndef __HREWRITE_UTILS_PRINT_H__
#define __HREWRITE_UTILS_PRINT_H__

#include <vector>
#include <tuple>
#include <variant>
#include <type_traits>

#include <iostream>

#include "hrewrite/utils/type_traits.hpp"
#include "hrewrite/utils/container.hpp"
#include "hrewrite/utils/variant.hpp"


namespace hrw {
  namespace utils {


    /////////////////////////////////////////////////////////////////////////////
    // TYPE PRINT
    /////////////////////////////////////////////////////////////////////////////

    inline std::string_view _get_function_name(std::string const & f_name) {
      std::size_t pos_end = f_name.find("(");
      std::size_t pos_begin = std::string_view(f_name.c_str(), pos_end).rfind(" ") + 1;

      return std::string_view(f_name.c_str() + pos_begin, pos_end - pos_begin);
    }

    inline std::string_view _get_class_name(std::string const & f_name) {
      std::string_view tmp = _get_function_name(f_name);
      std::size_t pos_colons = tmp.find("::");
      if(pos_colons == std::string::npos) {
        return "::";    
      } else {
        return std::string_view(tmp.data(), pos_colons);
      }
    }

    inline std::string_view _get_parameter_name(std::string const & f_name, std::string const & p_name) {
      std::size_t pos_start = f_name.find(p_name + " =");
      if(pos_start == std::string::npos) {
        return "::";    
      } else {
        pos_start += p_name.size() + 3;
        std::string_view tmp = std::string_view(f_name.c_str() + pos_start);
        std::size_t pos_end = tmp.find(";");
        if(pos_end == std::string_view::npos) {
          pos_end = tmp.size() - 1;
        }
        return std::string_view(tmp.data(), pos_end);
      }
    }

#if defined(__GNUC__) || defined(__clang__)
#define __FUNCTION_NAME__ hrw::utils::_get_function_name(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ hrw::utils::_get_class_name(__PRETTY_FUNCTION__)
#define __PARAMETER_TYPE__(__P) hrw::utils::_get_parameter_name(__PRETTY_FUNCTION__, __P)
#elif defined(_MSC_VER)
#define __FUNCTION_NAME__ hrw::utils::_get_function_name(__FUNCSIG__)
#define __CLASS_NAME__ hrw::utils::_get_class_name(__FUNCSIG__)
#define __PARAMETER_TYPE__(__P) hrw::utils::_get_parameter_name(__FUNCSIG__, __P)
#endif

    template<typename T> inline std::string type_name() {
      return std::string(__PARAMETER_TYPE__("T"));
    }



    /////////////////////////////////////////////////////////////////////////////
    // PRINT WITH CONTEXT
    /////////////////////////////////////////////////////////////////////////////

    template<typename CTX, typename DATA>
    struct print_with_ctx {
      print_with_ctx(CTX& ctx, DATA& data): m_ctx(ctx), m_data(data) {}
      CTX& m_ctx;
      DATA& m_data;
    };

    template<typename CTX, typename DATA>
    std::ostream& operator<<(std::ostream& os, print_with_ctx<CTX, DATA>& s) {
      s.m_data.print(os, s.m_ctx);
      return os;
    }

    template<typename CTX, typename DATA>
    std::ostream& operator<<(std::ostream& os, print_with_ctx<CTX, DATA>&& s) {
      s.m_data.print(os, s.m_ctx);
      return os;
    }


    /////////////////////////////////////////////////////////////////////////////
    // PRINT WITH WRAPPER (to avoid conflict with other implementations)
    /////////////////////////////////////////////////////////////////////////////

    struct wrap {
      template<typename T> static constexpr bool _has_operator_v = has_operator::left_shift_v<std::ostream&, std::ostream&, const T&>;

      template<typename T, std::enable_if_t<_has_operator_v<T>, bool> = true>
      static const T& print(const T& t) { return t; }

      template<typename T, std::enable_if_t<!(_has_operator_v<T>), bool> = true>
      static data_wrapper<const T&> print(const T& t) {
        using TT = data_wrapper<const T&>;
        return TT(t);
      }
    };


    template<typename T, std::enable_if_t<is_template_instance_v<std::remove_cv_t<std::remove_reference_t<T>>, std::pair>, bool> = true>
    std::ostream& operator<<(std::ostream& os, const data_wrapper<T>& v) {
      os << "(" << wrap::print(v.m_content.first) << ", " << wrap::print(v.m_content.second) << ")";
      return os;
    }

    template<typename T, std::enable_if_t<is_template_instance_v<std::remove_cv_t<std::remove_reference_t<T>>, std::vector>, bool> = true>
    std::ostream& operator<<(std::ostream& os, const data_wrapper<T>& v) {
      os << "[ ";
      for(auto& data: v.m_content) {
        os << wrap::print(data) << " ";
      }
      os << "]";
      return os;
    }

    template<typename T, std::enable_if_t<is_template_instance_v<std::remove_cv_t<std::remove_reference_t<T>>, std::variant>, bool> = true>
    std::ostream& operator<<(std::ostream& os, const data_wrapper<T>& v) {
      VISIT_SINGLE(visit_helper {
        [](const std::monostate&) {},
        [&os](auto& vv) { os << wrap::print(vv); }
      }, v.m_content);
      return os;
    }



    template<typename T, std::enable_if_t<is_template_instance_v<std::remove_cv_t<std::remove_reference_t<T>>, std::tuple>, bool> = true>
    std::ostream& operator<<(std::ostream& os, const data_wrapper<T>& v) {
      std::apply([&os](const auto& ... args) {
            os << "tuple( ";
            ((os << wrap::print(args) << " "), ...);
            os << ')';
      }, v.m_content);
      return os;
    }


  }
}


#endif // __HREWRITE_UTILS_PRINT_H__

