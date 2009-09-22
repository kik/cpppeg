#ifndef PEG_HPP
#define PEG_HPP

#include <string>
#include <vector>

#if !defined(BOOST_FUSION_INVOKE_MAX_ARITY)
#define BOOST_FUSION_INVOKE_MAX_ARITY 32
#endif

#include <boost/type.hpp>
#include <boost/type_traits.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/transform.hpp>

#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/view.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/functional.hpp>

#include <boost/fusion/adapted/mpl.hpp>

#include <boost/fusion/container/vector/convert.hpp>

namespace peg {

  template<class Constraint>
  struct chr {
    char v;
    chr(char vv = 0) : v(vv) {}
    operator char() const { return v; }
  };

  template<char s, char e>
  struct chr_range_constraint {
    bool operator()(char c) { return s <= c && c <= e; }
  };

  template<class T, class U>
  struct chr_or_constraint {
    bool operator()(char c) { return T()(c) || U()(c); }
  };

  template<char c>
  struct ch {
    typedef chr<chr_range_constraint<c, c> > type;
  };
  
  template<class T>
  struct constraint_of;

  template<class T>
  struct constraint_of<chr<T> > {
    typedef T type;
  };
  
  typedef chr<chr_range_constraint<'a', 'z'> > lower;
  typedef chr<chr_range_constraint<'A', 'Z'> > upper;
  typedef chr<chr_range_constraint<'0', '9'> > digit;
  typedef chr<chr_or_constraint<constraint_of<lower>::type, constraint_of<upper>::type > > alpha;
  typedef ch<' '>::type ws;

  struct eoi {
  };

  template<class T>
  struct container_for {
    typedef std::vector<T> type;
  };

  template<class T>
  struct container_for<chr<T> > {
    typedef std::string type;
  };
  
  template<class T, class Container = typename container_for<T>::type >
  struct rep : public Container {
  };

  template<class T, class Container = typename container_for<T>::type >
  struct replus : public Container {
  };

  template<class T>
  struct ptr : public T::result_type {
    typedef typename T::result_type base_type;
    ptr() : base_type() {}
    ptr(const base_type& v) : base_type(v) {}
    ptr(const ptr& v) : base_type(v) {}
  };

  template<class T>
  struct opt : public boost::shared_ptr<T> {
  };

  struct sor {
    bool left;
  };

  template<class T>
  struct term : public T {
  };

  namespace pi {
    template<class T, class Input>
    struct match_non_term {
      static bool match(T& t, Input& begin, Input end) {
        return parse_fun(t, begin, end);
      }
    };

    template<class T, class Input>
    struct match_elem : match_non_term<T, Input> {
    };

    template<class T, class Input>
    struct match_elem<chr<T>, Input> {
      static bool match(chr<T>& t, Input& begin, Input end) {
        if (begin != end) {
          char c = *begin;
          if (T()(c)) {
            t.v = c;
            ++begin;
            return true;
          }
        }
        return false;
      }
    };
    
    template<class Input>
    struct match_elem<eoi, Input> {
      static bool match(eoi& t, Input& begin, Input end) {
        return begin == end;
      }
    };

    template<class T, class C, class Input>
    struct match_elem<rep<T, C>, Input> {
      static bool match(rep<T>& t, Input& begin, Input end) {
        for (;;) {
          T s;
          if (!match_elem<T, Input>::match(s, begin, end)) break;
          t.push_back(s);
        }
        return true;
      }
    };

    template<class T, class C, class Input>
    struct match_elem<replus<T, C>, Input> {
      static bool match(replus<T>& t, Input& begin, Input end) {
        {
          T s;
          if (!match_elem<T, Input>::match(s, begin, end)) return false;
          t.push_back(s);
        }
        for (;;) {
          T s;
          if (!match_elem<T, Input>::match(s, begin, end)) break;
          t.push_back(s);
        }
        return true;
      }
    };
    
    template<class T, class Input>
    struct match_elem<opt<T>, Input> {
      static bool match(opt<T>& t, Input& begin, Input end) {
        T s;
        if (!match_elem<T, Input>::match(s, begin, end)) {
          t = 0;
        } else {
          t = new T(s);
        }
        return true;
      }
    };

    template<class Input>
    struct parse_fun_state {
      bool finished;
      bool skipping;
      Input start;
      Input cur;
      Input end;
    };

    template<class T, class Input>
    struct rule_step {
      static void run(T& t, parse_fun_state<Input> *state) {
        if (state->finished || state->skipping) {
        } else {
          state->skipping = !match_elem<T, Input>::match(t, state->cur, state->end);
        }
      }
    };
    
    template<class Input>
    struct rule_step<sor, Input> {
      static void run(sor& t, parse_fun_state<Input> *state) {
        if (state->finished) {
          t.left = true;
        } else if (state->skipping) {
          state->skipping = false;
          state->cur = state->start;
          t.left = false;
        } else {
          state->finished = true;
          t.left = true;
        }
      }
    };

    template<class Input>
    struct run_steps_fun {
      parse_fun_state<Input> *state;

      run_steps_fun(parse_fun_state<Input> *st) : state(st) {
      }
      
      template<class T>
      void operator()(T& t) const {
        rule_step<T, Input>::run(t, state);
      }
    };

    template<class T_param_list>
    struct parse_fun_types {
      typedef T_param_list src_param_list_type;
      typedef typename boost::mpl::transform<src_param_list_type,
        boost::remove_const<boost::remove_reference<boost::mpl::_1> > >::type param_list_type;
      typedef typename boost::fusion::result_of::as_vector<param_list_type>::type result_list_type;
    };

    template<class T, class Input>
    inline bool parse_fun_core(T& t, Input& begin, Input end) {
      parse_fun_state<Input> state;
      state.finished = false;
      state.skipping = false;
      state.cur = begin;
      state.start = begin;
      state.end = end;
      run_steps_fun<Input> step(&state);
      boost::fusion::for_each(t, step);
      if (!state.finished && state.skipping) {
        return false;
      } else {
        begin = state.cur;
        return true;
      }
    }
    
    struct parse_action_tag {};
    struct parse_ptr_action_tag {};
    struct parse_sequence_tag {};

    template<class T, class Tag>
    struct select_parse_fun;
    
    template<class T>
    struct select_parse_fun<T, parse_action_tag> {
      template<class Fun, class Input>
      static bool parse_fun_by_action(Fun fun, T& t, Input& begin, Input end) {
        typedef typename boost::function_types::parameter_types<Fun>::type param_list_1;
        typedef typename boost::mpl::pop_front<param_list_1>::type param_list;
        typedef parse_fun_types<param_list> types;
        typename types::result_list_type result;
        if (parse_fun_core(result, begin, end)) {
          boost::fusion::invoke(fun, boost::fusion::push_front(result, &t));
          return true;
        } else {
          return false;
        }
      }
      
      template<class Input>
      static bool parse_fun(T& t, Input& begin, Input end) {
        return parse_fun_by_action(&T::action, t, begin, end);
      }
    };

    template<class T>
    struct select_parse_fun<ptr<T>, parse_ptr_action_tag> {
      template<class Fun, class Input>
      static bool parse_fun_by_action(Fun fun, ptr<T>& t, Input& begin, Input end) {
        typedef typename boost::function_types::parameter_types<Fun>::type param_list;
        typedef parse_fun_types<param_list> types;
        typename types::result_list_type result;
        if (parse_fun_core(result, begin, end)) {
          t = boost::fusion::invoke(fun, result);
          return true;
        } else {
          return false;
        }
      }

      template<class Input>
      static bool parse_fun(ptr<T>& t, Input& begin, Input end) {
        return parse_fun_by_action(&T::action, t, begin, end);
      }
    };

    template<class T>
    struct select_parse_fun<T, parse_sequence_tag> {
      template<class Input>
      static bool parse_fun(T& t, Input& begin, Input end) {
        return parse_fun_core(t, begin, end);
      }
    };

    template<class T>
    struct parse_tag_of {
      typedef typename boost::mpl::if_<typename boost::fusion::traits::is_sequence<T>::type,
                                       parse_sequence_tag, parse_action_tag>::type type;
    };

    template<class T>
    struct parse_tag_of<ptr<T> > {
      typedef parse_ptr_action_tag type;
    };

    template<class T, class Input>
    inline bool parse_fun(T& t, Input& begin, Input end) {
      return select_parse_fun<T, typename parse_tag_of<T>::type>::parse_fun(t, begin, end);
    }
  }

  template<class T>
  struct parser {
    parser() {
    }

    template<class Input>
    bool parse(T& t, Input begin, Input end) {
      return pi::parse_fun(t, begin, end);
    }
  };
}
#endif

