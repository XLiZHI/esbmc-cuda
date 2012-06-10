#ifndef _UTIL_IREP2_H_
#define _UTIL_IREP2_H_

#include <stdarg.h>

#include <vector>

#define BOOST_SP_DISABLE_THREADS 1

#include <boost/mpl/if.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/crc.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/equal_to.hpp>

#include <irep.h>
#include <fixedbv.h>
#include <big-int/bigint.hh>
#include <dstring.h>

// XXXjmorse - abstract, access modifies, need consideration

#define forall_exprs(it, vect) \
  for (std::vector<expr2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_exprs(it, vect) \
  for (std::vector<expr2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_types(it, vect) \
  for (std::vector<type2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_types(it, vect) \
  for (std::vector<type2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_names(it, vect) \
  for (std::vector<irep_idt>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define Forall_names(it, vect) \
  for (std::vector<std::string>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

#define forall_operands2(it, ops, theexpr) \
  expr2t::expr_operands ops; \
  theexpr->list_operands(ops); \
  for (expr2t::expr_operands::const_iterator it = ops.begin(); \
       it != ops.end(); it++)

#define Forall_operands2(it, ops, theexpr) \
  expr2t::Expr_operands ops; \
  theexpr.get()->list_operands(ops); \
  for (expr2t::Expr_operands::iterator it = ops.begin(); \
       it != ops.end(); it++)

class prop_convt;
class type2t;
class expr2t;
class constant_array2t;

template <class T, int expid>
class irep_container : public boost::shared_ptr<T>
{
public:
  irep_container() : boost::shared_ptr<T>() {}

  template<class Y>
  explicit irep_container(Y *p) : boost::shared_ptr<T>(p)
    { assert(expid == -1 || p->expr_id == expid); }

  template<class Y>
  explicit irep_container(const Y *p) : boost::shared_ptr<T>(const_cast<Y *>(p))
    { assert(expid == -1 || p->expr_id == expid); }

  irep_container(const irep_container &ref)
    : boost::shared_ptr<T>(ref) {}

  template <class Y, int I>
  irep_container(const irep_container<Y, I> &ref)
    : boost::shared_ptr<T>(static_cast<const boost::shared_ptr<Y> &>(ref), boost::detail::polymorphic_cast_tag()) {}

  irep_container &operator=(irep_container const &ref)
  {
    boost::shared_ptr<T>::operator=(ref);
    T *p = boost::shared_ptr<T>::get();
    assert(expid == -1 || p == NULL || p->expr_id == expid);
    return *this;
  }

  template<class Y>
  irep_container & operator=(boost::shared_ptr<Y> const & r)
  {
    boost::shared_ptr<T>::operator=(r);
    T *p = boost::shared_ptr<T>::operator->();
    assert(expid == -1 || p == NULL || p->expr_id == expid);
    return *this;
  }

  template <class Y, int I>
  irep_container &operator=(const irep_container<Y, I> &ref)
  {
    *this = boost::shared_polymorphic_cast<T, Y>
            (static_cast<const boost::shared_ptr<Y> &>(ref));
    return *this;
  }

  const T &operator*() const
  {
    return *boost::shared_ptr<T>::get();
  }

  const T * operator-> () const // never throws
  {
    return boost::shared_ptr<T>::operator->();
  }

  const T * get() const // never throws
  {
    return boost::shared_ptr<T>::get();
  }

  T * get() // never throws
  {
    detach();
    return boost::shared_ptr<T>::get();
  }

  void detach(void)
  {
    if (this->use_count() == 1)
      return; // No point remunging oneself if we're the only user of the ptr.

    // Assign-operate ourself into containing a fresh copy of the data. This
    // creates a new reference counted object, and assigns it to ourself,
    // which causes the existing reference to be decremented.
    const T *foo = boost::shared_ptr<T>::get();
    *this = foo->clone();
    return;
  }
};

typedef boost::shared_ptr<type2t> type2tc;
typedef irep_container<expr2t, -1> expr2tc;

typedef std::pair<std::string,std::string> member_entryt;
typedef std::list<member_entryt> list_of_memberst;

template <class T>
static inline std::string type_to_string(const T &theval, int indent);

template <class T>
static inline bool do_type_cmp(const T &side1, const T &side2);

template <class T>
static inline int do_type_lt(const T &side1, const T &side2);

template <class T>
static inline void do_type_crc(const T &theval, boost::crc_32_type &crc);

template <class T>
static inline void do_type_list_operands(const T &theval,
                                         std::list<const expr2tc*> &inp);

template <class T>
static inline void do_type_list_operands(T& theval,
                                         std::list<expr2tc*> &inp);

/** Base class for all types */
class type2t
{
public:
  /** Enumeration identifying each sort of type.
   *  The idea being that we might (for whatever reason) at runtime need to fall
   *  back onto identifying a type through just one field, for some reason. It's
   *  also highly useful for debugging */
  enum type_ids {
    bool_id,
    empty_id,
    symbol_id,
    struct_id,
    union_id,
    code_id,
    array_id,
    pointer_id,
    unsignedbv_id,
    signedbv_id,
    fixedbv_id,
    string_id,
    end_type_id
  };

  // Class to be thrown when attempting to fetch the width of a symbolic type,
  // such as empty or code. Caller will have to worry about what to do about
  // that.
  class symbolic_type_excp {
  };

protected:
  type2t(type_ids id);
  type2t(const type2t &ref);

public:
  virtual void convert_smt_type(prop_convt &obj, void *&arg) const = 0;
  virtual unsigned int get_width(void) const = 0;
  bool operator==(const type2t &ref) const;
  bool operator!=(const type2t &ref) const;
  bool operator<(const type2t &ref) const;
  int ltchecked(const type2t &ref) const;
  std::string pretty(unsigned int indent = 0) const;
  void dump(void) const;
  uint32_t crc(void) const;
  bool cmpchecked(const type2t &ref) const;
  virtual bool cmp(const type2t &ref) const = 0;
  virtual int lt(const type2t &ref) const;
  virtual list_of_memberst tostring(unsigned int indent) const = 0;
  virtual void do_crc(boost::crc_32_type &crc) const;

  /** Instance of type_ids recording this types type. */
  type_ids type_id;
};


std::string get_type_id(const type2t &type);

static inline std::string get_type_id(const type2tc &type)
{
  return get_type_id(*type);
}

/** Base class for all expressions */
class expr2t
{
public:
  /** Enumeration identifying each sort of expr.
   *  The idea being to permit runtime identification of a type for debugging or
   *  otherwise. See type2t::type_ids. */
  enum expr_ids {
    constant_int_id,
    constant_fixedbv_id,
    constant_bool_id,
    constant_string_id,
    constant_struct_id,
    constant_union_id,
    constant_array_id,
    constant_array_of_id,
    symbol_id,
    typecast_id,
    if_id,
    equality_id,
    notequal_id,
    lessthan_id,
    greaterthan_id,
    lessthanequal_id,
    greaterthanequal_id,
    not_id,
    and_id,
    or_id,
    xor_id,
    implies_id,
    bitand_id,
    bitor_id,
    bitxor_id,
    bitnand_id,
    bitnor_id,
    bitnxor_id,
    bitnot_id,
    lshr_id,
    neg_id,
    abs_id,
    add_id,
    sub_id,
    mul_id,
    div_id,
    modulus_id,
    shl_id,
    ashr_id,
    dynamic_object_id, // Not converted in Z3, only in goto-symex
    same_object_id,
    pointer_offset_id,
    pointer_object_id,
    address_of_id,
    byte_extract_id,
    byte_update_id,
    with_id,
    member_id,
    index_id,
    zero_string_id,
    zero_length_string_id,
    isnan_id,
    overflow_id,
    overflow_cast_id,
    overflow_neg_id,
    unknown_id,
    invalid_id,
    null_object_id,
    dereference_id,
    valid_object_id,
    deallocated_obj_id,
    dynamic_size_id,
    sideeffect_id,
    code_block_id,
    code_assign_id,
    code_init_id,
    code_decl_id,
    code_printf_id,
    code_expression_id,
    code_return_id,
    code_skip_id,
    code_free_id,
    code_goto_id,
    object_descriptor_id,
    code_function_call_id,
    code_comma_id,
    invalid_pointer_id,
    buffer_size_id,
    to_bv_typecast_id,
    from_bv_typecast_id,
    end_expr_id
  };

  typedef std::list<const expr2tc*> expr_operands;
  typedef std::list<expr2tc*> Expr_operands;

protected:
  expr2t(const type2tc type, expr_ids id);
  expr2t(const expr2t &ref);

public:
  /** Clone method. Entirely self explanatory */
  virtual expr2tc clone(void) const = 0;

  virtual void convert_smt(prop_convt &obj, void *&arg) const = 0;

  bool operator==(const expr2t &ref) const;
  bool operator<(const expr2t &ref) const;
  bool operator!=(const expr2t &ref) const;
  int ltchecked(const expr2t &ref) const;
  std::string pretty(unsigned int indent = 0) const;
  unsigned long num_nodes(void) const;
  unsigned long depth(void) const;
  void dump(void) const;
  uint32_t crc(void) const;
  virtual bool cmp(const expr2t &ref) const;
  virtual int lt(const expr2t &ref) const;
  virtual list_of_memberst tostring(unsigned int indent) const = 0;
  virtual void do_crc(boost::crc_32_type &crc) const;
  virtual void list_operands(std::list<const expr2tc*> &inp) const = 0;

  // Caution - updating sub operands of an expr2t *must* always preserve type
  // correctness, as there's no way to check that an expr expecting a pointer
  // type operand *always* has a pointer type operand.
  // This list operands method should be protected; however it's required on
  // account of all those places where exprs are rewritten in place. Ideally,
  // "all those places" shouldn't exist in the future.
  virtual void list_operands(std::list<expr2tc*> &inp) = 0;
  virtual expr2t * clone_raw(void) const = 0;

  expr2tc simplify(void) const;
  // Shallow -> one level only. second indicates that this is its second
  // invocation, after a first invocation where all its operands aren't
  // simplified.
  virtual expr2tc do_simplify(bool second = false) const;

  /** Instance of expr_ids recording tihs exprs type. */
  const expr_ids expr_id;

  /** Type of this expr. All exprs have a type. */
  type2tc type;
};

std::string get_expr_id(const expr2t &expr);
static inline std::string get_expr_id(const expr2tc &expr)
{
  return get_expr_id(*expr);
}

// for "ESBMC templates",
namespace esbmct {

  // Template metaprogramming (vomit) -- define tag classes to instanciate
  // different class fields with different names and different types.

  #define field_name_macro(name) \
  template <class fieldtype> \
  struct name_class_##name { \
  public: \
    name_class_##name(fieldtype &someval) : name(someval) {} \
    name_class_##name(const name_class_##name &ref) : name(ref.name) {} \
    inline void tostring(list_of_memberst &membs, int indent) const \
    { return membs.push_back(member_entryt("" #name, \
                             type_to_string<fieldtype>(name, indent)));}\
    inline bool cmp(const name_class_##name &theother) const { \
      return do_type_cmp<fieldtype>(name, theother.name); }\
    inline int lt(const name_class_##name &theother) const { \
      return do_type_lt<fieldtype>(name, theother.name); }\
    inline void do_crc(boost::crc_32_type &crc) const { \
      do_type_crc<fieldtype>(name, crc); return; }\
    inline void list_operands(std::list<const expr2tc*> &inp) const { \
      do_type_list_operands<fieldtype>(name, inp); return; }\
    inline void list_operands(std::list<expr2tc*> &inp) { \
      do_type_list_operands<fieldtype>(name, inp); return; }\
    fieldtype name; \
  }; \
  template <class fieldtype> \
  struct name_##name { \
  public: \
    typedef name_class_##name<fieldtype> type; \
  };

  field_name_macro(constant_value);
  field_name_macro(value);
  field_name_macro(datatype_members);
  field_name_macro(name);
  field_name_macro(from);
  field_name_macro(cond);
  field_name_macro(true_value);
  field_name_macro(false_value);
  field_name_macro(side_1);
  field_name_macro(side_2);
  field_name_macro(notvalue);
  field_name_macro(ptr_obj);
  field_name_macro(big_endian);
  field_name_macro(source_value);
  field_name_macro(source_offset);
  field_name_macro(update_value);
  field_name_macro(update_field);
  field_name_macro(member);
  field_name_macro(index);
  field_name_macro(string);
  field_name_macro(bits);
  field_name_macro(initializer);
  field_name_macro(operand);
  field_name_macro(operands);
  field_name_macro(symbol_name);
  field_name_macro(members);
  field_name_macro(member_names);
  field_name_macro(width);
  field_name_macro(integer_bits);
  field_name_macro(subtype);
  field_name_macro(array_size);
  field_name_macro(size_is_infinite);
  field_name_macro(instance);
  field_name_macro(invalid);
  field_name_macro(unknown);
  field_name_macro(size);
  field_name_macro(alloctype);
  field_name_macro(kind);
  field_name_macro(arguments);
  field_name_macro(ret_type);
  field_name_macro(ellipsis);
  field_name_macro(argument_names);
  field_name_macro(target);
  field_name_macro(source);
  field_name_macro(object);
  field_name_macro(offset);
  field_name_macro(ret);
  field_name_macro(function);
  #undef field_name_macro

  // Multiple empty name tags are required to avoid inheretance of the same type

  #define empty_names_macro(num) \
  class name_empty_##num; \
  struct name_class_empty_##num { \
  public: \
    name_class_empty_##num() { } \
    name_class_empty_##num(const name_class_empty_##num &ref \
                           __attribute__((unused))) {} \
    name_class_empty_##num(const name_empty_##num &ref \
                           __attribute__((unused))) {} \
    inline void tostring(list_of_memberst &membs __attribute__((unused)),\
                         int indent __attribute__((unused))) const\
    { return; } \
    inline bool cmp(const name_class_empty_##num &ref __attribute__((unused)))\
    const { return true; }\
    inline int lt(const name_class_empty_##num &ref __attribute__((unused)))\
    const { return 0; }\
    inline void do_crc(boost::crc_32_type &crc __attribute__((unused))) const \
    { return; }\
    inline void list_operands(std::list<const expr2tc*> &inp\
                              __attribute__((unused))) const \
    { return; } \
    inline void list_operands(std::list<expr2tc*> &inp\
                              __attribute__((unused))) \
    { return; } \
  }; \
  class name_empty_##num { \
  public: \
    typedef name_class_empty_##num type; \
  };

  empty_names_macro(1);
  empty_names_macro(2);
  empty_names_macro(3);
  empty_names_macro(4);
  #undef empty_names_macro

  // Type tags
  #define field_type_macro(name, thetype) \
  class name { \
    public: \
    typedef thetype type; \
  };

  field_type_macro(int_type_tag, int);
  field_type_macro(uint_type_tag, unsigned int);
  field_type_macro(bool_type_tag, bool);
  field_type_macro(bigint_type_tag, BigInt);
  field_type_macro(expr2tc_type_tag, expr2tc);
  field_type_macro(fixedbv_type_tag, fixedbvt);
  field_type_macro(expr2tc_vec_type_tag, std::vector<expr2tc>);
  field_type_macro(irepidt_type_tag, irep_idt);
  field_type_macro(type2tc_vec_type_tag, std::vector<type2tc>);
  field_type_macro(irepidt_vec_type_tag, std::vector<irep_idt>);
  field_type_macro(type2tc_type_tag, type2tc);
  #undef field_type_macro

  #define member_record_macro(thename, thetype, fieldname) \
  struct thename { \
    typedef fieldname<thetype::type>::type fieldtype; \
    typedef thetype::type type; \
    const static int enabled = true; \
  };

  member_record_macro(constant_int_value, int_type_tag, name_constant_value);
  member_record_macro(constant_bigint_value, bigint_type_tag,
                      name_constant_value);
  member_record_macro(fixedbv_value, fixedbv_type_tag, name_value);
  member_record_macro(constant_bool_value, bool_type_tag, name_constant_value);
  member_record_macro(irepidt_value, irepidt_type_tag, name_value);
  member_record_macro(expr2tc_vec_datatype_members, expr2tc_vec_type_tag,
                      name_datatype_members);
  member_record_macro(expr2tc_initializer, expr2tc_type_tag, name_initializer);
  member_record_macro(irepidt_name, irepidt_type_tag, name_name);
  member_record_macro(expr2tc_from, expr2tc_type_tag, name_from);
  member_record_macro(expr2tc_cond, expr2tc_type_tag, name_cond);
  member_record_macro(expr2tc_true_value, expr2tc_type_tag, name_true_value);
  member_record_macro(expr2tc_false_value, expr2tc_type_tag, name_false_value);
  member_record_macro(expr2tc_side_1, expr2tc_type_tag, name_side_1);
  member_record_macro(expr2tc_side_2, expr2tc_type_tag, name_side_2);
  member_record_macro(expr2tc_value, expr2tc_type_tag, name_value);
  member_record_macro(expr2tc_ptr_obj, expr2tc_type_tag, name_ptr_obj);
  member_record_macro(bool_big_endian, bool_type_tag, name_big_endian);
  member_record_macro(expr2tc_source_value, expr2tc_type_tag,
                      name_source_value);
  member_record_macro(expr2tc_source_offset, expr2tc_type_tag,
                      name_source_offset);
  member_record_macro(expr2tc_update_value, expr2tc_type_tag,
                      name_update_value);
  member_record_macro(expr2tc_update_field, expr2tc_type_tag,
                      name_update_field);
  member_record_macro(irepidt_member, irepidt_type_tag, name_member);
  member_record_macro(expr2tc_index, expr2tc_type_tag, name_index);
  member_record_macro(expr2tc_string, expr2tc_type_tag, name_string);
  member_record_macro(expr2tc_operand, expr2tc_type_tag, name_operand);
  member_record_macro(uint_bits, uint_type_tag, name_bits);
  member_record_macro(irepidt_symbol_name, irepidt_type_tag, name_symbol_name);
  member_record_macro(type2tc_vec_members, type2tc_vec_type_tag, name_members);
  member_record_macro(irepidt_vec_member_names, irepidt_vec_type_tag,
                      name_member_names);
  member_record_macro(uint_width, uint_type_tag, name_width);
  member_record_macro(uint_int_bits, uint_type_tag, name_integer_bits);
  member_record_macro(type2tc_subtype, type2tc_type_tag, name_subtype);
  member_record_macro(expr2tc_array_size, expr2tc_type_tag, name_array_size);
  member_record_macro(bool_size_is_inf, bool_type_tag, name_size_is_infinite);
  member_record_macro(bool_invalid, bool_type_tag, name_invalid);
  member_record_macro(bool_unknown, bool_type_tag, name_unknown);
  member_record_macro(expr2tc_instance, expr2tc_type_tag, name_instance);
  member_record_macro(expr2tc_size, expr2tc_type_tag, name_size);
  member_record_macro(type2tc_alloctype, type2tc_type_tag, name_alloctype);
  member_record_macro(uint_kind, uint_type_tag, name_kind);
  member_record_macro(type2tc_vec_args, type2tc_vec_type_tag, name_arguments);
  member_record_macro(type2tc_ret_type, type2tc_type_tag, name_ret_type);
  member_record_macro(bool_ellipsis, bool_type_tag, name_ellipsis);
  member_record_macro(irepidt_vec_arg_names, irepidt_vec_type_tag,
                      name_argument_names);
  member_record_macro(expr2tc_vec_operands, expr2tc_vec_type_tag,
                      name_operands);
  member_record_macro(expr2tc_target, expr2tc_type_tag, name_target);
  member_record_macro(expr2tc_source, expr2tc_type_tag, name_source);
  member_record_macro(expr2tc_object, expr2tc_type_tag, name_object);
  member_record_macro(expr2tc_offset, expr2tc_type_tag, name_offset);
  member_record_macro(expr2tc_function, expr2tc_type_tag, name_function);
  member_record_macro(expr2tc_ret, expr2tc_type_tag, name_ret);
  member_record_macro(irepidt_target, irepidt_type_tag, name_target);
  #undef member_record_macro

  template <class thename>
  struct blank_value {
    typedef typename thename::type fieldtype;
    typedef thename type;
    const static thename defaultval;
    const static int enabled = false;
  };

  template <class derived,
            class field1 = esbmct::blank_value<esbmct::name_empty_1>,
            class field2 = esbmct::blank_value<esbmct::name_empty_2>,
            class field3 = esbmct::blank_value<esbmct::name_empty_3>,
            class field4 = esbmct::blank_value<esbmct::name_empty_4> >
  class expr :
    public expr2t,
    public field1::fieldtype,
    public field2::fieldtype,
    public field3::fieldtype,
    public field4::fieldtype
  {
  public:

    expr(const type2tc type, expr_ids id,
        typename field1::type arg1 = field1::defaultval,
        typename field2::type arg2 = field2::defaultval,
        typename field3::type arg3 = field3::defaultval,
        typename field4::type arg4 = field4::defaultval)
      : expr2t(type, id),
        field1::fieldtype(arg1),
        field2::fieldtype(arg2),
        field3::fieldtype(arg3),
        field4::fieldtype(arg4)
    {};

    expr(const expr &ref)
      : expr2t(ref),
        field1::fieldtype(ref),
        field2::fieldtype(ref),
        field3::fieldtype(ref),
        field4::fieldtype(ref)
    {}

    virtual void convert_smt(prop_convt &obj, void *&arg) const;
    virtual expr2tc clone(void) const;
    virtual list_of_memberst tostring(unsigned int indent) const;
    virtual bool cmp(const expr2t &ref) const;
    virtual int lt(const expr2t &ref) const;
    virtual void do_crc(boost::crc_32_type &crc) const;
    virtual void list_operands(std::list<const expr2tc*> &inp) const;
  protected:
    virtual void list_operands(std::list<expr2tc*> &inp);
    virtual expr2t *clone_raw(void) const;
  };

  class blank_method_operand {
  };

  const unsigned int num_type_fields = 4;

  template <class derived, class subclass,
     typename field1_type = const expr2t::expr_ids, class field1_class = expr2t,
     field1_type field1_class::*field1_ptr = &field1_class::expr_id,
     typename field2_type = const expr2t::expr_ids, class field2_class = expr2t,
     field2_type field2_class::*field2_ptr = &field2_class::expr_id,
     typename field3_type = const expr2t::expr_ids, class field3_class = expr2t,
     field3_type field3_class::*field3_ptr = &field3_class::expr_id,
     typename field4_type = const expr2t::expr_ids, class field4_class = expr2t,
     field4_type field4_class::*field4_ptr = &field4_class::expr_id>
  class expr_methods : public subclass
  {
    class dummy_type_tag {
      typedef int type;
    };

  public:
    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id,
                 typename boost::lazy_enable_if<boost::fusion::result_of::equal_to<subclass,type2t>, arbitary >::type* = NULL)
      : subclass(t, id) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id,
                 const field1_type &arg1,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field1_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field2_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field2_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field3_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field3_type,expr2t::expr_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field4_type,expr2t::expr_ids> >, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2, arg3) { }

    template <class arbitary = dummy_type_tag>
    expr_methods(const type2tc &t, expr2t::expr_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,expr2t::expr_ids>, arbitary >::type* = NULL)
      : subclass(t, id, arg1, arg2, arg3, arg4) { }

    expr_methods(const expr_methods<derived, subclass,
                                    field1_type, field1_class, field1_ptr,
                                    field2_type, field2_class, field2_ptr,
                                    field3_type, field3_class, field3_ptr,
                                    field4_type, field4_class, field4_ptr> &ref)
      : subclass(ref) { }

    virtual void convert_smt(prop_convt &obj, void *&arg) const;
    virtual expr2tc clone(void) const;
    virtual list_of_memberst tostring(unsigned int indent) const;
    virtual bool cmp(const expr2t &ref) const;
    virtual int lt(const expr2t &ref) const;
    virtual void do_crc(boost::crc_32_type &crc) const;
    virtual void list_operands(std::list<const expr2tc*> &inp) const;
  protected:
    virtual void list_operands(std::list<expr2tc*> &inp);
    virtual expr2t *clone_raw(void) const;
  };

  template <class derived, class subclass,
          typename field1_type = type2t::type_ids, class field1_class = type2t,
          field1_type field1_class::*field1_ptr = &field1_class::type_id,
          typename field2_type = type2t::type_ids, class field2_class = type2t,
          field2_type field2_class::*field2_ptr = &field2_class::type_id,
          typename field3_type = type2t::type_ids, class field3_class = type2t,
          field3_type field3_class::*field3_ptr = &field3_class::type_id,
          typename field4_type = type2t::type_ids, class field4_class = type2t,
          field4_type field4_class::*field4_ptr = &field4_class::type_id>
  class type_methods : public subclass
  {
    class dummy_type_tag {
      typedef int type;
    };

  public:
    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id,
                 typename boost::lazy_enable_if<boost::fusion::result_of::equal_to<subclass,type2t>, arbitary >::type* = NULL)
      : subclass(id) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id,
                 const field1_type &arg1,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field1_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field2_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field2_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field3_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field3_type,type2t::type_ids>, arbitary >::type* = NULL,
                 typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<field4_type,type2t::type_ids> >, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2, arg3) { }

    template <class arbitary = dummy_type_tag>
    type_methods(type2t::type_ids id, const field1_type &arg1,
                 const field2_type &arg2, const field3_type &arg3,
                 const field4_type &arg4,
                 typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<field4_type,type2t::type_ids>, arbitary >::type* = NULL)
      : subclass(id, arg1, arg2, arg3, arg4) { }

    type_methods(const type_methods<derived, subclass,
                                    field1_type, field1_class, field1_ptr,
                                    field2_type, field2_class, field2_ptr,
                                    field3_type, field3_class, field3_ptr,
                                    field4_type, field4_class, field4_ptr> &ref)
      : subclass(ref) { }

    virtual void convert_smt_type(prop_convt &obj, void *&arg) const;
    virtual type2tc clone(void) const;
    virtual list_of_memberst tostring(unsigned int indent) const;
    virtual bool cmp(const type2t &ref) const;
    virtual int lt(const type2t &ref) const;
    virtual void do_crc(boost::crc_32_type &crc) const;
  };
}; // esbmct

// So - make some type definitions for the different types we're going to be
// working with. This is to avoid the repeated use of template names in later
// definitions.

// Start with forward class definitions

class bool_type2t;
class empty_type2t;
class symbol_type2t;
class struct_type2t;
class union_type2t;
class bv_type2t;
class unsignedbv_type2t;
class signedbv_type2t;
class code_type2t;
class array_type2t;
class pointer_type2t;
class fixedbv_type2t;
class string_type2t;

// We also require in advance, the actual classes that store type data.

class symbol_type_data : public type2t
{
public:
  symbol_type_data(type2t::type_ids id, const dstring sym_name) :
    type2t (id), symbol_name(sym_name) {}
  symbol_type_data(const symbol_type_data &ref) :
    type2t (ref), symbol_name(ref.symbol_name) { }

  irep_idt symbol_name;
};

class struct_union_data : public type2t
{
public:
  struct_union_data(type2t::type_ids id, const std::vector<type2tc> &membs,
                     const std::vector<irep_idt> &names, const irep_idt &n)
    : type2t(id), members(membs), member_names(names), name(n) { }
  struct_union_data(const struct_union_data &ref)
    : type2t(ref), members(ref.members), member_names(ref.member_names),
      name(ref.name) { }

  const std::vector<type2tc> & get_structure_members(void) const;
  const std::vector<irep_idt> & get_structure_member_names(void) const;
  const irep_idt & get_structure_name(void) const;

  std::vector<type2tc> members;
  std::vector<irep_idt> member_names;
  irep_idt name;
};

class bv_data : public type2t
{
public:
  bv_data(type2t::type_ids id, unsigned int w) : type2t(id), width(w) { }
  bv_data(const bv_data &ref) : type2t(ref), width(ref.width) { }

  virtual unsigned int get_width(void) const;

  unsigned int width;
};

class code_data : public type2t
{
public:
  code_data(type2t::type_ids id, const std::vector<type2tc> &args,
            const type2tc &ret, const std::vector<irep_idt> &names, bool e)
    : type2t(id), arguments(args), ret_type(ret), argument_names(names),
      ellipsis(e) { }
  code_data(const code_data &ref)
    : type2t(ref), arguments(ref.arguments), ret_type(ref.ret_type),
      argument_names(ref.argument_names), ellipsis(ref.ellipsis) { }

  virtual unsigned int get_width(void) const;

  std::vector<type2tc> arguments;
  type2tc ret_type;
  std::vector<irep_idt> argument_names;
  bool ellipsis;
};

class array_data : public type2t
{
public:
  array_data(type2t::type_ids id, const type2tc &st, const expr2tc &sz, bool i)
    : type2t(id), subtype(st), array_size(sz), size_is_infinite(i) { }
  array_data(const array_data &ref)
    : type2t(ref), subtype(ref.subtype), array_size(ref.array_size),
      size_is_infinite(ref.size_is_infinite) { }

  type2tc subtype;
  expr2tc array_size;
  bool size_is_infinite;
};

class pointer_data : public type2t
{
public:
  pointer_data(type2t::type_ids id, const type2tc &st)
    : type2t(id), subtype(st) { }
  pointer_data(const pointer_data &ref)
    : type2t(ref), subtype(ref.subtype) { }

  type2tc subtype;
};

class fixedbv_data : public type2t
{
public:
  fixedbv_data(type2t::type_ids id, unsigned int w, unsigned int ib)
    : type2t(id), width(w), integer_bits(ib) { }
  fixedbv_data(const fixedbv_data &ref)
    : type2t(ref), width(ref.width), integer_bits(ref.integer_bits) { }

  unsigned int width;
  unsigned int integer_bits;
};

class string_data : public type2t
{
public:
  string_data(type2t::type_ids id, unsigned int w)
    : type2t(id), width(w) { }
  string_data(const string_data &ref)
    : type2t(ref), width(ref.width) { }

  unsigned int width;
};

// Then give them a typedef name

typedef esbmct::type_methods<bool_type2t, type2t> bool_type_methods;
typedef esbmct::type_methods<empty_type2t, type2t> empty_type_methods;
typedef esbmct::type_methods<symbol_type2t, symbol_type_data, irep_idt,
        symbol_type_data, &symbol_type_data::symbol_name> symbol_type_methods;
typedef esbmct::type_methods<struct_type2t, struct_union_data,
    std::vector<type2tc>, struct_union_data, &struct_union_data::members,
    std::vector<irep_idt>, struct_union_data, &struct_union_data::member_names,
    irep_idt, struct_union_data, &struct_union_data::name>
    struct_type_methods;
typedef esbmct::type_methods<union_type2t, struct_union_data,
    std::vector<type2tc>, struct_union_data, &struct_union_data::members,
    std::vector<irep_idt>, struct_union_data, &struct_union_data::member_names,
    irep_idt, struct_union_data, &struct_union_data::name>
    union_type_methods;
typedef esbmct::type_methods<unsignedbv_type2t, bv_data, unsigned int, bv_data,
    &bv_data::width> unsignedbv_type_methods;
typedef esbmct::type_methods<signedbv_type2t, bv_data, unsigned int, bv_data,
    &bv_data::width> signedbv_type_methods;
typedef esbmct::type_methods<code_type2t, code_data,
    std::vector<type2tc>, code_data, &code_data::arguments,
    type2tc, code_data, &code_data::ret_type,
    std::vector<irep_idt>, code_data, &code_data::argument_names,
    bool, code_data, &code_data::ellipsis>
    code_type_methods;
typedef esbmct::type_methods<array_type2t, array_data,
    type2tc, array_data, &array_data::subtype,
    expr2tc, array_data, &array_data::array_size,
    bool, array_data, &array_data::size_is_infinite>
    array_type_methods;
typedef esbmct::type_methods<pointer_type2t, pointer_data,
    type2tc, pointer_data, &pointer_data::subtype>
    pointer_type_methods;
typedef esbmct::type_methods<fixedbv_type2t, fixedbv_data,
    unsigned int, fixedbv_data, &fixedbv_data::width,
    unsigned int, fixedbv_data, &fixedbv_data::integer_bits>
    fixedbv_type_methods;
typedef esbmct::type_methods<string_type2t, string_data,
    unsigned int, string_data, &string_data::width>
    string_type_methods;

/** Boolean type. No additional data */
class bool_type2t : public bool_type_methods
{
public:
  bool_type2t(void) : bool_type_methods (bool_id) {}
  bool_type2t(const bool_type2t &ref) : bool_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type. For void pointers and the like, with no type. No extra data */
class empty_type2t : public empty_type_methods
{
public:
  empty_type2t(void) : empty_type_methods(empty_id) {}
  empty_type2t(const empty_type2t &ref) : empty_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Symbol type. Temporary, prior to linking up types after parsing, or when
 *  a struct/array contains a recursive pointer to its own type. */

class symbol_type2t : public symbol_type_methods
{
public:
  symbol_type2t(const dstring sym_name) :
    symbol_type_methods(symbol_id, sym_name) { }
  symbol_type2t(const symbol_type2t &ref) :
    symbol_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class struct_type2t : public struct_type_methods
{
public:
  struct_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : struct_type_methods(struct_id, members, memb_names, name) {}
  struct_type2t(const struct_type2t &ref) : struct_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class union_type2t : public union_type_methods
{
public:
  union_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : union_type_methods(union_id, members, memb_names, name) {}
  union_type2t(const union_type2t &ref) : union_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class unsignedbv_type2t : public unsignedbv_type_methods
{
public:
  unsignedbv_type2t(unsigned int width)
    : unsignedbv_type_methods(unsignedbv_id, width) { }
  unsignedbv_type2t(const unsignedbv_type2t &ref)
    : unsignedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class signedbv_type2t : public signedbv_type_methods
{
public:
  signedbv_type2t(signed int width)
    : signedbv_type_methods(signedbv_id, width) { }
  signedbv_type2t(const signedbv_type2t &ref)
    : signedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type. For void pointers and the like, with no type. No extra data */
class code_type2t : public code_type_methods
{
public:
  code_type2t(const std::vector<type2tc> &args, const type2tc &ret_type,
              const std::vector<irep_idt> &names, bool e)
    : code_type_methods(code_id, args, ret_type, names, e)
  { assert(args.size() == names.size()); }
  code_type2t(const code_type2t &ref) : code_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Array type. Comes with a subtype of the array and a size that might be
 *  constant, might be nondeterministic. */
class array_type2t : public array_type_methods
{
public:
  array_type2t(const type2tc subtype, const expr2tc size, bool inf)
    : array_type_methods (array_id, subtype, size, inf) { }
  array_type2t(const array_type2t &ref)
    : array_type_methods(ref) { }

  virtual unsigned int get_width(void) const;

  // Exception for invalid manipulations of an infinitely sized array. No actual
  // data stored.
  class inf_sized_array_excp {
  };

  // Exception for invalid manipultions of dynamically sized arrays. No actual
  // data stored.
  class dyn_sized_array_excp {
  public:
    dyn_sized_array_excp(const expr2tc _size) : size(_size) {}
    expr2tc size;
  };

  static std::string field_names[esbmct::num_type_fields];
};

/** Pointer type. Simply has a subtype, of what it points to. No other
 *  attributes */
class pointer_type2t : public pointer_type_methods
{
public:
  pointer_type2t(const type2tc subtype)
    : pointer_type_methods(pointer_id, subtype) { }
  pointer_type2t(const pointer_type2t &ref)
    : pointer_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class fixedbv_type2t : public fixedbv_type_methods
{
public:
  fixedbv_type2t(unsigned int width, unsigned int integer)
    : fixedbv_type_methods(fixedbv_id, width, integer) { }
  fixedbv_type2t(const fixedbv_type2t &ref)
    : fixedbv_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class string_type2t : public string_type_methods
{
public:
  string_type2t(unsigned int elements)
    : string_type_methods(string_id, elements) { }
  string_type2t(const string_type2t &ref)
    : string_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

// Generate some "is-this-a-blah" macros, and type conversion macros. This is
// fine in terms of using/ keywords in syntax, because the preprocessor
// preprocesses everything out. One more used to C++ templates might raise their
// eyebrows at using the preprocessor; nuts to you, this works.
#ifdef NDEBUG
#define dynamic_cast static_cast
#endif
#define type_macros(name) \
  inline bool is_##name##_type(const type2tc &t) \
    { return t->type_id == type2t::name##_id; } \
  inline const name##_type2t & to_##name##_type(const type2tc &t) \
    { return dynamic_cast<const name##_type2t &> (*t.get()); } \
  inline name##_type2t & to_##name##_type(type2tc &t) \
    { return dynamic_cast<name##_type2t &> (*t.get()); }

type_macros(bool);
type_macros(empty);
type_macros(symbol);
type_macros(struct);
type_macros(union);
type_macros(code);
type_macros(array);
type_macros(pointer);
type_macros(unsignedbv);
type_macros(signedbv);
type_macros(fixedbv);
type_macros(string);
#undef type_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

inline bool is_bv_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id); }

inline bool is_number_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id ||
          t->type_id == type2t::fixedbv_id); }

// And now, some more utilities.
class type_poolt {
public:
  type_poolt(void);

  type2tc bool_type;
  type2tc empty_type;

  const type2tc &get_bool() const { return bool_type; }
  const type2tc &get_empty() const { return empty_type; }

  // For other types, have a pool of them for quick lookup.
  std::map<const typet, type2tc> struct_map;
  std::map<const typet, type2tc> union_map;
  std::map<const typet, type2tc> array_map;
  std::map<const typet, type2tc> pointer_map;
  std::map<const typet, type2tc> unsignedbv_map;
  std::map<const typet, type2tc> signedbv_map;
  std::map<const typet, type2tc> fixedbv_map;
  std::map<const typet, type2tc> string_map;
  std::map<const typet, type2tc> symbol_map;
  std::map<const typet, type2tc> code_map;

  // And refs to some of those for /really/ quick lookup;
  const type2tc *uint8;
  const type2tc *uint16;
  const type2tc *uint32;
  const type2tc *uint64;
  const type2tc *int8;
  const type2tc *int16;
  const type2tc *int32;
  const type2tc *int64;

  // Some accessors.
  const type2tc &get_struct(const typet &val);
  const type2tc &get_union(const typet &val);
  const type2tc &get_array(const typet &val);
  const type2tc &get_pointer(const typet &val);
  const type2tc &get_unsignedbv(const typet &val);
  const type2tc &get_signedbv(const typet &val);
  const type2tc &get_fixedbv(const typet &val);
  const type2tc &get_string(const typet &val);
  const type2tc &get_symbol(const typet &val);
  const type2tc &get_code(const typet &val);

  const type2tc &get_uint(unsigned int size);
  const type2tc &get_int(unsigned int size);

  const type2tc &get_uint8() const { return *uint8; }
  const type2tc &get_uint16() const { return *uint16; }
  const type2tc &get_uint32() const { return *uint32; }
  const type2tc &get_uint64() const { return *uint64; }
  const type2tc &get_int8() const { return *int8; }
  const type2tc &get_int16() const { return *int16; }
  const type2tc &get_int32() const { return *int32; }
  const type2tc &get_int64() const { return *int64; }
};

extern type_poolt type_pool;

// Start of definitions for expressions. Forward decs,

class constant2t;
class constant_int2t;
class constant_fixedbv2t;
class constant_bool2t;
class constant_string2t;
class constant_datatype2t;
class constant_struct2t;
class constant_union2t;
class constant_array2t;
class constant_array_of2t;
class symbol2t;
class typecast2t;
class to_bv_typecast2t;
class from_bv_typecast2t;
class if2t;
class equality2t;
class notequal2t;
class lessthan2t;
class greaterthan2t;
class lessthanequal2t;
class greaterthanequal2t;
class not2t;
class and2t;
class or2t;
class xor2t;
class implies2t;
class bitand2t;
class bitor2t;
class bitxor2t;
class bitnand2t;
class bitnor2t;
class bitnxor2t;
class lshr2t;
class bitnot2t;
class neg2t;

// Data definitions.

class constant2t : public expr2t
{
public:
  constant2t(const type2tc &t, expr2t::expr_ids id) : expr2t(t, id) { }
  constant2t(const constant2t &ref) : expr2t(ref) { }
};

class constant_int_data : public constant2t
{
public:
  constant_int_data(const type2tc &t, expr2t::expr_ids id, const BigInt &bint)
    : constant2t(t, id), constant_value(bint) { }
  constant_int_data(const constant_int_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  BigInt constant_value;
};

class constant_fixedbv_data : public constant2t
{
public:
  constant_fixedbv_data(const type2tc &t, expr2t::expr_ids id,
                        const fixedbvt &fbv)
    : constant2t(t, id), value(fbv) { }
  constant_fixedbv_data(const constant_fixedbv_data &ref)
    : constant2t(ref), value(ref.value) { }

  fixedbvt value;
};

class constant_datatype_data : public constant2t
{
public:
  constant_datatype_data(const type2tc &t, expr2t::expr_ids id,
                         const std::vector<expr2tc> &m)
    : constant2t(t, id), datatype_members(m) { }
  constant_datatype_data(const constant_datatype_data &ref)
    : constant2t(ref), datatype_members(ref.datatype_members) { }

  std::vector<expr2tc> datatype_members;
};

class constant_bool_data : public constant2t
{
public:
  constant_bool_data(const type2tc &t, expr2t::expr_ids id, bool value)
    : constant2t(t, id), constant_value(value) { }
  constant_bool_data(const constant_bool_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  bool constant_value;
};

class constant_array_of_data : public constant2t
{
public:
  constant_array_of_data(const type2tc &t, expr2t::expr_ids id, expr2tc value)
    : constant2t(t, id), initializer(value) { }
  constant_array_of_data(const constant_array_of_data &ref)
    : constant2t(ref), initializer(ref.initializer) { }

  expr2tc initializer;
};

class constant_string_data : public constant2t
{
public:
  constant_string_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : constant2t(t, id), value(v) { }
  constant_string_data(const constant_string_data &ref)
    : constant2t(ref), value(ref.value) { }

  irep_idt value;
};

class symbol_data : public expr2t
{
public:
  symbol_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : expr2t(t, id), name(v) { }
  symbol_data(const symbol_data &ref)
    : expr2t(ref), name(ref.name) { }

  irep_idt name;
};

class typecast_data : public expr2t
{
public:
  typecast_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), from(v) { }
  typecast_data(const typecast_data &ref)
    : expr2t(ref), from(ref.from) { }

  expr2tc from;
};

class if_data : public expr2t
{
public:
  if_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &c,
                const expr2tc &tv, const expr2tc &fv)
    : expr2t(t, id), cond(c), true_value(tv), false_value(fv) { }
  if_data(const if_data &ref)
    : expr2t(ref), cond(ref.cond), true_value(ref.true_value),
      false_value(ref.false_value) { }

  expr2tc cond;
  expr2tc true_value;
  expr2tc false_value;
};

class relation_data : public expr2t
{
  public:
  relation_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
                const expr2tc &s2)
    : expr2t(t, id), side_1(s1), side_2(s2) { }
  relation_data(const relation_data &ref)
    : expr2t(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class logical_ops : public expr2t
{
public:
  logical_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  logical_ops(const logical_ops &ref)
    : expr2t(ref) { }
};

class not_data : public logical_ops
{
public:
  not_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : logical_ops(t, id), value(v) { }
  not_data(const not_data &ref)
    : logical_ops(ref), value(ref.value) { }

  expr2tc value;
};

class logic_2ops : public logical_ops
{
public:
  logic_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
             const expr2tc &s2)
    : logical_ops(t, id), side_1(s1), side_2(s2) { }
  logic_2ops(const logic_2ops &ref)
    : logical_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class bitops : public expr2t
{
public:
  bitops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  bitops(const bitops &ref)
    : expr2t(ref) { }
};

class bitnot_data : public bitops
{
public:
  bitnot_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : bitops(t, id), value(v) { }
  bitnot_data(const bitnot_data &ref)
    : bitops(ref), value(ref.value) { }

  expr2tc value;
};

class bit_2ops : public bitops
{
public:
  bit_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
           const expr2tc &s2)
    : bitops(t, id), side_1(s1), side_2(s2) { }
  bit_2ops(const bit_2ops &ref)
    : bitops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

class arith_ops : public expr2t
{
public:
  arith_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  arith_ops(const arith_ops &ref)
    : expr2t(ref) { }
};

class arith_1op : public arith_ops
{
public:
  arith_1op(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v)
    : arith_ops(t, id), value(v) { }
  arith_1op(const arith_1op &ref)
    : arith_ops(ref), value(ref.value) { }

  expr2tc value;
};

class arith_2ops : public arith_ops
{
public:
  arith_2ops(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v1,
             const expr2tc &v2)
    : arith_ops(t, id), side_1(v1), side_2(v2) { }
  arith_2ops(const arith_2ops &ref)
    : arith_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;
};

// Give everything a typedef name

typedef esbmct::expr_methods<constant_int2t, constant_int_data,
        BigInt, constant_int_data, &constant_int_data::constant_value>
        constant_int_expr_methods;
typedef esbmct::expr_methods<constant_fixedbv2t, constant_fixedbv_data,
        fixedbvt, constant_fixedbv_data, &constant_fixedbv_data::value>
        constant_fixedbv_expr_methods;
typedef esbmct::expr_methods<constant_struct2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_struct_expr_methods;
typedef esbmct::expr_methods<constant_union2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_union_expr_methods;
typedef esbmct::expr_methods<constant_array2t, constant_datatype_data,
        std::vector<expr2tc>, constant_datatype_data,
        &constant_datatype_data::datatype_members>
        constant_array_expr_methods;
typedef esbmct::expr_methods<constant_bool2t, constant_bool_data,
        bool, constant_bool_data, &constant_bool_data::constant_value>
        constant_bool_expr_methods;
typedef esbmct::expr_methods<constant_array_of2t, constant_array_of_data,
        expr2tc, constant_array_of_data, &constant_array_of_data::initializer>
        constant_array_of_expr_methods;
typedef esbmct::expr_methods<constant_string2t, constant_string_data,
        irep_idt, constant_string_data, &constant_string_data::value>
        constant_string_expr_methods;
typedef esbmct::expr_methods<symbol2t, symbol_data,
        irep_idt, symbol_data, &symbol_data::name>
        symbol_expr_methods;
typedef esbmct::expr_methods<typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        typecast_expr_methods;
typedef esbmct::expr_methods<to_bv_typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        to_bv_typecast_expr_methods;
typedef esbmct::expr_methods<from_bv_typecast2t, typecast_data,
        expr2tc, typecast_data, &typecast_data::from>
        from_bv_typecast_expr_methods;
typedef esbmct::expr_methods<if2t, if_data,
        expr2tc, if_data, &if_data::cond,
        expr2tc, if_data, &if_data::true_value,
        expr2tc, if_data, &if_data::false_value>
        if_expr_methods;
typedef esbmct::expr_methods<equality2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        equality_expr_methods;
typedef esbmct::expr_methods<notequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        notequal_expr_methods;
typedef esbmct::expr_methods<lessthan2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        lessthan_expr_methods;
typedef esbmct::expr_methods<greaterthan2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        greaterthan_expr_methods;
typedef esbmct::expr_methods<lessthanequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        lessthanequal_expr_methods;
typedef esbmct::expr_methods<greaterthanequal2t, relation_data,
        expr2tc, relation_data, &relation_data::side_1,
        expr2tc, relation_data, &relation_data::side_2>
        greaterthanequal_expr_methods;
typedef esbmct::expr_methods<not2t, not_data,
        expr2tc, not_data, &not_data::value>
        not_expr_methods;
typedef esbmct::expr_methods<and2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        and_expr_methods;
typedef esbmct::expr_methods<or2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        or_expr_methods;
typedef esbmct::expr_methods<xor2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        xor_expr_methods;
typedef esbmct::expr_methods<implies2t, logic_2ops,
        expr2tc, logic_2ops, &logic_2ops::side_1,
        expr2tc, logic_2ops, &logic_2ops::side_2>
        implies_expr_methods;
typedef esbmct::expr_methods<bitand2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitand_expr_methods;
typedef esbmct::expr_methods<bitor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitor_expr_methods;
typedef esbmct::expr_methods<bitxor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitxor_expr_methods;
typedef esbmct::expr_methods<bitnand2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnand_expr_methods;
typedef esbmct::expr_methods<bitnor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnor_expr_methods;
typedef esbmct::expr_methods<bitnxor2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        bitnxor_expr_methods;
typedef esbmct::expr_methods<lshr2t, bit_2ops,
        expr2tc, bit_2ops, &bit_2ops::side_1,
        expr2tc, bit_2ops, &bit_2ops::side_2>
        lshr_expr_methods;
typedef esbmct::expr_methods<bitnot2t, bitnot_data,
        expr2tc, bitnot_data, &bitnot_data::value>
        bitnot_expr_methods;
typedef esbmct::expr_methods<neg2t, arith_1op,
        expr2tc, arith_1op, &arith_1op::value>
        neg_expr_methods;

/** Constant integer class. Records a constant integer of an arbitary
 *  precision */
class constant_int2t : public constant_int_expr_methods
{
public:
  constant_int2t(const type2tc &type, const BigInt &input)
    : constant_int_expr_methods(type, constant_int_id, input) { }
  constant_int2t(const constant_int2t &ref)
    : constant_int_expr_methods(ref) { }

  /** Accessor for fetching native int of this constant */
  unsigned long as_ulong(void) const;
  long as_long(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant fixedbv class. Records a floating point number in what I assume
 *  to be mantissa/exponent form, but which is described throughout CBMC code
 *  as fraction/integer parts. */
class constant_fixedbv2t : public constant_fixedbv_expr_methods
{
public:
  constant_fixedbv2t(const type2tc &type, const fixedbvt &value)
    : constant_fixedbv_expr_methods(type, constant_fixedbv_id, value) { }
  constant_fixedbv2t(const constant_fixedbv2t &ref)
    : constant_fixedbv_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_bool2t : public constant_bool_expr_methods
{
public:
  constant_bool2t(bool value)
    : constant_bool_expr_methods(type_pool.get_bool(), constant_bool_id, value)
      { }
  constant_bool2t(const constant_bool2t &ref)
    : constant_bool_expr_methods(ref) { }

  bool is_true(void) const;
  bool is_false(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant class for string constants. */
class constant_string2t : public constant_string_expr_methods
{
public:
  constant_string2t(const type2tc &type, const irep_idt &stringref)
    : constant_string_expr_methods(type, constant_string_id, stringref) { }
  constant_string2t(const constant_string2t &ref)
    : constant_string_expr_methods(ref) { }

  /** Convert string to a constant length array */
  expr2tc to_array(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class constant_struct2t : public constant_struct_expr_methods
{
public:
  constant_struct2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_struct_expr_methods (type, constant_struct_id, members) { }
  constant_struct2t(const constant_struct2t &ref)
    : constant_struct_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_union2t : public constant_union_expr_methods
{
public:
  constant_union2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_union_expr_methods (type, constant_union_id, members) { }
  constant_union2t(const constant_union2t &ref)
    : constant_union_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class constant_array2t : public constant_array_expr_methods
{
public:
  constant_array2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_array_expr_methods(type, constant_array_id, members) { }
  constant_array2t(const constant_array2t &ref)
    : constant_array_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class constant_array_of2t : public constant_array_of_expr_methods
{
public:
  constant_array_of2t(const type2tc &type, const expr2tc &init)
    : constant_array_of_expr_methods(type, constant_array_of_id, init) { }
  constant_array_of2t(const constant_array_of2t &ref)
    : constant_array_of_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class symbol2t : public symbol_expr_methods
{
public:
  symbol2t(const type2tc &type, const irep_idt &init)
    : symbol_expr_methods(type, symbol_id, init) { }
  symbol2t(const symbol2t &ref)
    : symbol_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class typecast2t : public typecast_expr_methods
{
public:
  typecast2t(const type2tc &type, const expr2tc &from)
    : typecast_expr_methods(type, typecast_id, from) { }
  typecast2t(const typecast2t &ref)
    : typecast_expr_methods(ref){}
  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

// Typecast, but explicitly either to or from a bit vector. This prevents any
// semantic conversion of floats to/from bits.
class to_bv_typecast2t : public to_bv_typecast_expr_methods
{
public:
  to_bv_typecast2t(const type2tc &type, const expr2tc &from)
    : to_bv_typecast_expr_methods(type, to_bv_typecast_id, from) { }
  to_bv_typecast2t(const to_bv_typecast2t &ref)
    : to_bv_typecast_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class from_bv_typecast2t : public from_bv_typecast_expr_methods
{
public:
  from_bv_typecast2t(const type2tc &type, const expr2tc &from)
    : from_bv_typecast_expr_methods(type, from_bv_typecast_id, from) { }
  from_bv_typecast2t(const from_bv_typecast2t &ref)
    : from_bv_typecast_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

class if2t : public if_expr_methods
{
public:
  if2t(const type2tc &type, const expr2tc &cond, const expr2tc &trueval,
       const expr2tc &falseval)
    : if_expr_methods(type, if_id, cond, trueval, falseval) {}
  if2t(const if2t &ref)
    : if_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class equality2t : public equality_expr_methods
{
public:
  equality2t(const expr2tc &v1, const expr2tc &v2)
    : equality_expr_methods(type_pool.get_bool(), equality_id, v1, v2) {}
  equality2t(const equality2t &ref)
    : equality_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class notequal2t : public notequal_expr_methods
{
public:
  notequal2t(const expr2tc &v1, const expr2tc &v2)
    : notequal_expr_methods(type_pool.get_bool(), notequal_id, v1, v2) {}
  notequal2t(const notequal2t &ref)
    : notequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lessthan2t : public lessthan_expr_methods
{
public:
  lessthan2t(const expr2tc &v1, const expr2tc &v2)
    : lessthan_expr_methods(type_pool.get_bool(), lessthan_id, v1, v2) {}
  lessthan2t(const lessthan2t &ref)
    : lessthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class greaterthan2t : public greaterthan_expr_methods
{
public:
  greaterthan2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthan_expr_methods(type_pool.get_bool(), greaterthan_id, v1, v2) {}
  greaterthan2t(const greaterthan2t &ref)
    : greaterthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lessthanequal2t : public lessthanequal_expr_methods
{
public:
  lessthanequal2t(const expr2tc &v1, const expr2tc &v2)
  : lessthanequal_expr_methods(type_pool.get_bool(), lessthanequal_id, v1, v2){}
  lessthanequal2t(const lessthanequal2t &ref)
  : lessthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class greaterthanequal2t : public greaterthanequal_expr_methods
{
public:
  greaterthanequal2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthanequal_expr_methods(type_pool.get_bool(), greaterthanequal_id,
                                    v1, v2) {}
  greaterthanequal2t(const greaterthanequal2t &ref)
    : greaterthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class not2t : public not_expr_methods
{
public:
  not2t(const expr2tc &val)
  : not_expr_methods(type_pool.get_bool(), not_id, val) {}
  not2t(const not2t &ref)
  : not_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class and2t : public and_expr_methods
{
public:
  and2t(const expr2tc &s1, const expr2tc &s2)
  : and_expr_methods(type_pool.get_bool(), and_id, s1, s2) {}
  and2t(const and2t &ref)
  : and_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class or2t : public or_expr_methods
{
public:
  or2t(const expr2tc &s1, const expr2tc &s2)
  : or_expr_methods(type_pool.get_bool(), or_id, s1, s2) {}
  or2t(const or2t &ref)
  : or_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class xor2t : public xor_expr_methods
{
public:
  xor2t(const expr2tc &s1, const expr2tc &s2)
  : xor_expr_methods(type_pool.get_bool(), xor_id, s1, s2) {}
  xor2t(const xor2t &ref)
  : xor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class implies2t : public implies_expr_methods
{
public:
  implies2t(const expr2tc &s1, const expr2tc &s2)
  : implies_expr_methods(type_pool.get_bool(), implies_id, s1, s2) {}
  implies2t(const implies2t &ref)
  : implies_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitand2t : public bitand_expr_methods
{
public:
  bitand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitand_expr_methods(t, bitand_id, s1, s2) {}
  bitand2t(const bitand2t &ref)
  : bitand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitor2t : public bitor_expr_methods
{
public:
  bitor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitor_expr_methods(t, bitor_id, s1, s2) {}
  bitor2t(const bitor2t &ref)
  : bitor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitxor2t : public bitxor_expr_methods
{
public:
  bitxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitxor_expr_methods(t, bitxor_id, s1, s2) {}
  bitxor2t(const bitxor2t &ref)
  : bitxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnand2t : public bitnand_expr_methods
{
public:
  bitnand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnand_expr_methods(t, bitnand_id, s1, s2) {}
  bitnand2t(const bitnand2t &ref)
  : bitnand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnor2t : public bitnor_expr_methods
{
public:
  bitnor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnor_expr_methods(t, bitnor_id, s1, s2) {}
  bitnor2t(const bitnor2t &ref)
  : bitnor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnxor2t : public bitnxor_expr_methods
{
public:
  bitnxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnxor_expr_methods(t, bitnxor_id, s1, s2) {}
  bitnxor2t(const bitnxor2t &ref)
  : bitnxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class bitnot2t : public bitnot_expr_methods
{
public:
  bitnot2t(const type2tc &type, const expr2tc &v)
    : bitnot_expr_methods(type, bitnot_id, v) {}
  bitnot2t(const bitnot2t &ref)
    : bitnot_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class lshr2t : public lshr_expr_methods
{
public:
  lshr2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : lshr_expr_methods(t, lshr_id, s1, s2) {}
  lshr2t(const lshr2t &ref)
  : lshr_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class neg2t : public neg_expr_methods
{
public:
  neg2t(const type2tc &type, const expr2tc &val)
    : neg_expr_methods(type, neg_id, val) {}
  neg2t(const neg2t &ref)
    : neg_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

class abs2t : public esbmct::expr<abs2t,esbmct::expr2tc_value>
{
public:
  abs2t(const type2tc &type, const expr2tc &val)
    : esbmct::expr<abs2t, esbmct::expr2tc_value> (type, abs_id, val) {}
  abs2t(const abs2t &ref)
    : esbmct::expr<abs2t, esbmct::expr2tc_value> (ref) {}
};
template class esbmct::expr<abs2t, esbmct::expr2tc_value>;

class add2t : public esbmct::expr<add2t, esbmct::expr2tc_side_1,
                                       esbmct::expr2tc_side_2>
{
public:
  add2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<add2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, add_id, v1, v2) {}
  add2t(const add2t &ref)
    : esbmct::expr<add2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<add2t, esbmct::expr2tc_side_1,
                                 esbmct::expr2tc_side_2>;

class sub2t : public esbmct::expr<sub2t, esbmct::expr2tc_side_1,
                                       esbmct::expr2tc_side_2>
{
public:
  sub2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<sub2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, sub_id, v1, v2) {}
  sub2t(const sub2t &ref)
    : esbmct::expr<sub2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<sub2t, esbmct::expr2tc_side_1,
                                 esbmct::expr2tc_side_2>;

class mul2t : public esbmct::expr<mul2t, esbmct::expr2tc_side_1,
                                       esbmct::expr2tc_side_2>
{
public:
  mul2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<mul2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, mul_id, v1, v2) {}
  mul2t(const mul2t &ref)
    : esbmct::expr<mul2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<mul2t, esbmct::expr2tc_side_1,
                                 esbmct::expr2tc_side_2>;

class div2t : public esbmct::expr<div2t, esbmct::expr2tc_side_1,
                                       esbmct::expr2tc_side_2>
{
public:
  div2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<div2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, div_id, v1, v2) {}
  div2t(const div2t &ref)
    : esbmct::expr<div2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<div2t, esbmct::expr2tc_side_1,
                                 esbmct::expr2tc_side_2>;

class modulus2t : public esbmct::expr<modulus2t, esbmct::expr2tc_side_1,
                                               esbmct::expr2tc_side_2>
{
public:
  modulus2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<modulus2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, modulus_id, v1, v2) {}
  modulus2t(const modulus2t &ref)
    : esbmct::expr<modulus2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<modulus2t, esbmct::expr2tc_side_1,
                                     esbmct::expr2tc_side_2>;

class shl2t : public esbmct::expr<shl2t, esbmct::expr2tc_side_1,
                                       esbmct::expr2tc_side_2>
{
public:
  shl2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<shl2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, shl_id, v1, v2) {}
  shl2t(const shl2t &ref)
    : esbmct::expr<shl2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<shl2t, esbmct::expr2tc_side_1,
                                 esbmct::expr2tc_side_2>;

class ashr2t : public esbmct::expr<ashr2t, esbmct::expr2tc_side_1,
                                         esbmct::expr2tc_side_2>
{
public:
  ashr2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<ashr2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type, ashr_id, v1, v2) {}
  ashr2t(const ashr2t &ref)
    : esbmct::expr<ashr2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<ashr2t, esbmct::expr2tc_side_1,
                                  esbmct::expr2tc_side_2>;

class same_object2t : public esbmct::expr<same_object2t, esbmct::expr2tc_side_1,
                                                       esbmct::expr2tc_side_2>
{
public:
  same_object2t(const expr2tc &v1, const expr2tc &v2)
    : esbmct::expr<same_object2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (type_pool.get_bool(), same_object_id, v1, v2) {}
  same_object2t(const same_object2t &ref)
    : esbmct::expr<same_object2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<same_object2t, esbmct::expr2tc_side_1,
                                         esbmct::expr2tc_side_2>;


class pointer_offset2t : public esbmct::expr<pointer_offset2t,
                                           esbmct::expr2tc_ptr_obj>
{
public:
  pointer_offset2t(const type2tc &type, const expr2tc &ptrobj)
    : esbmct::expr<pointer_offset2t, esbmct::expr2tc_ptr_obj>
      (type, pointer_offset_id, ptrobj) {}
  pointer_offset2t(const pointer_offset2t &ref)
    : esbmct::expr<pointer_offset2t, esbmct::expr2tc_ptr_obj> (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<pointer_offset2t, esbmct::expr2tc_ptr_obj>;

class pointer_object2t : public esbmct::expr<pointer_object2t,
                                           esbmct::expr2tc_ptr_obj>
{
public:
  pointer_object2t(const type2tc &type, const expr2tc &ptrobj)
    : esbmct::expr<pointer_object2t, esbmct::expr2tc_ptr_obj>
      (type, pointer_object_id, ptrobj) {}
  pointer_object2t(const pointer_object2t &ref)
    : esbmct::expr<pointer_object2t, esbmct::expr2tc_ptr_obj> (ref) {}
};
template class esbmct::expr<pointer_object2t, esbmct::expr2tc_ptr_obj>;

class address_of2t : public esbmct::expr<address_of2t,
                                           esbmct::expr2tc_ptr_obj>
{
public:
  address_of2t(const type2tc &subtype, const expr2tc &ptrobj)
    : esbmct::expr<address_of2t, esbmct::expr2tc_ptr_obj>
      (type2tc(new pointer_type2t(subtype)), address_of_id, ptrobj) {}
  address_of2t(const address_of2t &ref)
    : esbmct::expr<address_of2t, esbmct::expr2tc_ptr_obj> (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<address_of2t, esbmct::expr2tc_ptr_obj>;

class byte_extract2t : public esbmct::expr<byte_extract2t,
                                           esbmct::bool_big_endian,
                                           esbmct::expr2tc_source_value,
                                           esbmct::expr2tc_source_offset>
{
public:
  byte_extract2t(const type2tc &type, bool is_big_endian, const expr2tc &source,
                 const expr2tc &offset)
    : esbmct::expr<byte_extract2t, esbmct::bool_big_endian,
                 esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset>
      (type, byte_extract_id, is_big_endian, source, offset) {}
  byte_extract2t(const byte_extract2t &ref)
    : esbmct::expr<byte_extract2t, esbmct::bool_big_endian,
                 esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset>
      (ref) {}
};
template class esbmct::expr<byte_extract2t, esbmct::bool_big_endian,
                  esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset>;

class byte_update2t : public esbmct::expr<byte_update2t,
                                           esbmct::bool_big_endian,
                                           esbmct::expr2tc_source_value,
                                           esbmct::expr2tc_source_offset,
                                           esbmct::expr2tc_update_value>
{
public:
  byte_update2t(const type2tc &type, bool is_big_endian, const expr2tc &source,
                 const expr2tc &offset, const expr2tc &updateval)
    : esbmct::expr<byte_update2t, esbmct::bool_big_endian,
                 esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset,
                 esbmct::expr2tc_update_value>
      (type, byte_update_id, is_big_endian, source, offset, updateval) {}
  byte_update2t(const byte_update2t &ref)
    : esbmct::expr<byte_update2t, esbmct::bool_big_endian,
                 esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset,
                 esbmct::expr2tc_update_value>
      (ref) {}
};
template class esbmct::expr<byte_update2t, esbmct::bool_big_endian,
                  esbmct::expr2tc_source_value, esbmct::expr2tc_source_offset,
                  esbmct::expr2tc_update_value>;

class with2t : public esbmct::expr<with2t, esbmct::expr2tc_source_value,
                                         esbmct::expr2tc_update_field,
                                         esbmct::expr2tc_update_value>
{
public:
  with2t(const type2tc &type, const expr2tc &source, const expr2tc &field,
         const expr2tc &value)
    : esbmct::expr<with2t, esbmct::expr2tc_source_value,
                         esbmct::expr2tc_update_field,
                         esbmct::expr2tc_update_value>
      (type, with_id, source, field, value) {}
  with2t(const with2t &ref)
    : esbmct::expr<with2t, esbmct::expr2tc_source_value,
                         esbmct::expr2tc_update_field,
                         esbmct::expr2tc_update_value> (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<with2t, esbmct::expr2tc_source_value,
                                  esbmct::expr2tc_update_field,
                                  esbmct::expr2tc_update_value>;

class member2t : public esbmct::expr<member2t, esbmct::expr2tc_source_value,
                                             esbmct::irepidt_member>
{
public:
  member2t(const type2tc &type, const expr2tc &source, const irep_idt &memb)
    : esbmct::expr<member2t, esbmct::expr2tc_source_value, esbmct::irepidt_member>
      (type, member_id, source, memb) {}
  member2t(const member2t &ref)
    : esbmct::expr<member2t, esbmct::expr2tc_source_value, esbmct::irepidt_member>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<member2t, esbmct::expr2tc_source_value,
                                    esbmct::irepidt_member>;

class index2t : public esbmct::expr<index2t, esbmct::expr2tc_source_value,
                                           esbmct::expr2tc_index>
{
public:
  index2t(const type2tc &type, const expr2tc &source, const expr2tc &index)
    : esbmct::expr<index2t, esbmct::expr2tc_source_value, esbmct::expr2tc_index>
      (type, index_id, source, index) {}
  index2t(const index2t &ref)
    : esbmct::expr<index2t, esbmct::expr2tc_source_value, esbmct::expr2tc_index>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<index2t, esbmct::expr2tc_source_value,
                                   esbmct::expr2tc_index>;

class zero_string2t : public esbmct::expr<zero_string2t, esbmct::expr2tc_string>
{
public:
  zero_string2t(const expr2tc &string)
    : esbmct::expr<zero_string2t, esbmct::expr2tc_string>
      (type_pool.get_bool(), zero_string_id, string) {}
  zero_string2t(const zero_string2t &ref)
    : esbmct::expr<zero_string2t, esbmct::expr2tc_string>
      (ref) {}
};
template class esbmct::expr<zero_string2t, esbmct::expr2tc_string>;

class zero_length_string2t : public esbmct::expr<zero_length_string2t, esbmct::expr2tc_string>
{
public:
  zero_length_string2t(const expr2tc &string)
    : esbmct::expr<zero_length_string2t, esbmct::expr2tc_string>
      (type_pool.get_bool(), zero_length_string_id, string) {}
  zero_length_string2t(const zero_length_string2t &ref)
    : esbmct::expr<zero_length_string2t, esbmct::expr2tc_string>
      (ref) {}
};
template class esbmct::expr<zero_length_string2t, esbmct::expr2tc_string>;

class isnan2t : public esbmct::expr<isnan2t, esbmct::expr2tc_value>
{
public:
  isnan2t(const expr2tc &value)
    : esbmct::expr<isnan2t, esbmct::expr2tc_value>
      (type_pool.get_bool(), isnan_id, value) {}
  isnan2t(const isnan2t &ref)
    : esbmct::expr<isnan2t, esbmct::expr2tc_value>
      (ref) {}
};
template class esbmct::expr<isnan2t, esbmct::expr2tc_value>;

/** Check whether operand overflows. Operand must be either add, subtract,
 *  or multiply. XXXjmorse - in the future we should ensure the type of the
 *  operand is the expected type result of the operation. That way we can tell
 *  whether to do a signed or unsigned over/underflow test. */

class overflow2t : public esbmct::expr<overflow2t, esbmct::expr2tc_operand>
{
public:
  overflow2t(const expr2tc &operand)
    : esbmct::expr<overflow2t, esbmct::expr2tc_operand>
      (type_pool.get_bool(), overflow_id, operand) {}
  overflow2t(const overflow2t &ref)
    : esbmct::expr<overflow2t, esbmct::expr2tc_operand>
      (ref) {}
  virtual expr2tc do_simplify(bool second) const;
};
template class esbmct::expr<overflow2t, esbmct::expr2tc_operand>;

class overflow_cast2t : public esbmct::expr<overflow_cast2t,
                                          esbmct::uint_bits,
                                          esbmct::expr2tc_operand>
{
public:
  overflow_cast2t(const expr2tc &operand, unsigned int bits)
    : esbmct::expr<overflow_cast2t, esbmct::uint_bits, esbmct::expr2tc_operand>
      (type_pool.get_bool(), overflow_cast_id, bits, operand) {}
  overflow_cast2t(const overflow_cast2t &ref)
    : esbmct::expr<overflow_cast2t, esbmct::uint_bits, esbmct::expr2tc_operand>
      (ref) {}
};
template class esbmct::expr<overflow_cast2t, esbmct::uint_bits,
                                           esbmct::expr2tc_operand>;

class overflow_neg2t : public esbmct::expr<overflow_neg2t,
                                         esbmct::expr2tc_operand>
{
public:
  overflow_neg2t(const expr2tc &operand)
    : esbmct::expr<overflow_neg2t, esbmct::expr2tc_operand>
      (type_pool.get_bool(), overflow_neg_id, operand) {}
  overflow_neg2t(const overflow_neg2t &ref)
    : esbmct::expr<overflow_neg2t, esbmct::expr2tc_operand>
      (ref) {}
};
template class esbmct::expr<overflow_neg2t, esbmct::expr2tc_operand>;

class unknown2t : public esbmct::expr<unknown2t>
{
public:
  unknown2t(const type2tc &type)
    : esbmct::expr<unknown2t> (type, unknown_id) {}
  unknown2t(const unknown2t &ref)
    : esbmct::expr<unknown2t> (ref) {}
};
template class esbmct::expr<unknown2t>;

class invalid2t : public esbmct::expr<invalid2t>
{
public:
  invalid2t(const type2tc &type)
    : esbmct::expr<invalid2t> (type, invalid_id) {}
  invalid2t(const invalid2t &ref)
    : esbmct::expr<invalid2t> (ref) {}
};
template class esbmct::expr<invalid2t>;

class null_object2t : public esbmct::expr<null_object2t>
{
public:
  null_object2t(const type2tc &type)
    : esbmct::expr<null_object2t> (type, null_object_id) {}
  null_object2t(const null_object2t &ref)
    : esbmct::expr<null_object2t> (ref) {}
};
template class esbmct::expr<null_object2t>;

class dynamic_object2t : public esbmct::expr<dynamic_object2t,
                                             esbmct::expr2tc_instance,
                                             esbmct::bool_invalid,
                                             esbmct::bool_unknown>
{
public:
  dynamic_object2t(const type2tc &type, const expr2tc inst,
                   bool inv, bool uknown)
    : esbmct::expr<dynamic_object2t, esbmct::expr2tc_instance,
                   esbmct::bool_invalid, esbmct::bool_unknown>
      (type, dynamic_object_id, inst, inv, uknown) {}
  dynamic_object2t(const dynamic_object2t &ref)
    : esbmct::expr<dynamic_object2t, esbmct::expr2tc_instance,
                   esbmct::bool_invalid, esbmct::bool_unknown> (ref) {}
};
template class esbmct::expr<dynamic_object2t, esbmct::expr2tc_instance,
                                   esbmct::bool_invalid, esbmct::bool_unknown>;

class dereference2t : public esbmct::expr<dereference2t, esbmct::expr2tc_value>
{
public:
  dereference2t(const type2tc &type, const expr2tc &operand)
    : esbmct::expr<dereference2t, esbmct::expr2tc_value>
      (type, dereference_id, operand) {}
  dereference2t(const dereference2t &ref)
    : esbmct::expr<dereference2t, esbmct::expr2tc_value> (ref) {}
};
template class esbmct::expr<dereference2t, esbmct::expr2tc_value>;

class valid_object2t : public esbmct::expr<valid_object2t,
                                           esbmct::expr2tc_value>
{
public:
  valid_object2t(const expr2tc &operand)
    : esbmct::expr<valid_object2t, esbmct::expr2tc_value>
      (type_pool.get_bool(), valid_object_id, operand) {}
  valid_object2t(const valid_object2t &ref)
    : esbmct::expr<valid_object2t, esbmct::expr2tc_value> (ref) {}
};
template class esbmct::expr<valid_object2t, esbmct::expr2tc_value>;

class deallocated_obj2t : public esbmct::expr<deallocated_obj2t,
                                              esbmct::expr2tc_value>
{
public:
  deallocated_obj2t(const expr2tc &operand)
    : esbmct::expr<deallocated_obj2t, esbmct::expr2tc_value>
      (type_pool.get_bool(), deallocated_obj_id, operand) {}
  deallocated_obj2t(const deallocated_obj2t &ref)
    : esbmct::expr<deallocated_obj2t, esbmct::expr2tc_value> (ref) {}
};
template class esbmct::expr<deallocated_obj2t, esbmct::expr2tc_value>;

class dynamic_size2t : public esbmct::expr<dynamic_size2t,
                                              esbmct::expr2tc_value>
{
public:
  dynamic_size2t(const expr2tc &operand)
    : esbmct::expr<dynamic_size2t, esbmct::expr2tc_value>
      (type_pool.get_bool(), dynamic_size_id, operand) {}
  dynamic_size2t(const dynamic_size2t &ref)
    : esbmct::expr<dynamic_size2t, esbmct::expr2tc_value> (ref) {}
};
template class esbmct::expr<dynamic_size2t, esbmct::expr2tc_value>;

class sideeffect2t : public esbmct::expr<sideeffect2t,
                                         esbmct::expr2tc_operand,
                                         esbmct::expr2tc_size,
                                         esbmct::type2tc_alloctype,
                                         esbmct::uint_kind>
{
public:
  enum allockind {
    malloc,
    cpp_new,
    cpp_new_arr,
    nondet
  };

  sideeffect2t(const type2tc &t, const expr2tc &oper, const expr2tc &sz,
               const type2tc &alloct, allockind k)
    : esbmct::expr<sideeffect2t, esbmct::expr2tc_operand,
                   esbmct::expr2tc_size, esbmct::type2tc_alloctype,
                   esbmct::uint_kind>
      (t, sideeffect_id, oper, sz, alloct, k) {}
  sideeffect2t(const sideeffect2t &ref)
    : esbmct::expr<sideeffect2t, esbmct::expr2tc_operand,
                   esbmct::expr2tc_size, esbmct::type2tc_alloctype,
                   esbmct::uint_kind> (ref) {}

};
template class esbmct::expr<sideeffect2t, esbmct::expr2tc_operand,
                            esbmct::expr2tc_size, esbmct::type2tc_alloctype,
                            esbmct::uint_kind>;

class code_block2t : public esbmct::expr<code_block2t,
                                         esbmct::expr2tc_vec_operands>
{
public:
  code_block2t(const std::vector<expr2tc> &operands)
    : esbmct::expr<code_block2t, esbmct::expr2tc_vec_operands>
      (type_pool.get_empty(), code_block_id, operands) {}
  code_block2t(const code_block2t &ref)
    : esbmct::expr<code_block2t, esbmct::expr2tc_vec_operands> (ref) {}
};
template class esbmct::expr<code_block2t, esbmct::expr2tc_vec_operands>;

class code_assign2t : public esbmct::expr<code_assign2t,
                                          esbmct::expr2tc_target,
                                          esbmct::expr2tc_source>
{
public:
  code_assign2t(const expr2tc &target, const expr2tc &source)
    : esbmct::expr<code_assign2t, esbmct::expr2tc_target,esbmct::expr2tc_source>
      (type_pool.get_empty(), code_assign_id, target, source) {}
  code_assign2t(const code_assign2t &ref)
    : esbmct::expr<code_assign2t, esbmct::expr2tc_target,esbmct::expr2tc_source>
      (ref) {}
};
template class esbmct::expr<code_assign2t, esbmct::expr2tc_target,
                            esbmct::expr2tc_source>;

class code_init2t : public esbmct::expr<code_init2t,
                                          esbmct::expr2tc_target,
                                          esbmct::expr2tc_source>
{
public:
  code_init2t(const expr2tc &target, const expr2tc &source)
    : esbmct::expr<code_init2t, esbmct::expr2tc_target,esbmct::expr2tc_source>
      (type_pool.get_empty(), code_init_id, target, source) {}
  code_init2t(const code_init2t &ref)
    : esbmct::expr<code_init2t, esbmct::expr2tc_target,esbmct::expr2tc_source>
      (ref) {}
};
template class esbmct::expr<code_init2t, esbmct::expr2tc_target,
                            esbmct::expr2tc_source>;

class code_decl2t : public esbmct::expr<code_decl2t, esbmct::irepidt_value>
{
public:
  code_decl2t(const type2tc &t, const irep_idt &name)
    : esbmct::expr<code_decl2t, esbmct::irepidt_value> (t, code_decl_id, name){}
  code_decl2t(const code_decl2t &ref)
    : esbmct::expr<code_decl2t, esbmct::irepidt_value>
      (ref) {}
};
template class esbmct::expr<code_decl2t, esbmct::irepidt_value>;

class code_printf2t : public esbmct::expr<code_printf2t,
                                          esbmct::expr2tc_vec_operands>
{
public:
  code_printf2t(const std::vector<expr2tc> &opers)
    : esbmct::expr<code_printf2t, esbmct::expr2tc_vec_operands>
      (type_pool.get_empty(), code_printf_id, opers) {}
  code_printf2t(const code_printf2t &ref)
    : esbmct::expr<code_printf2t, esbmct::expr2tc_vec_operands>
      (ref) {}
};
template class esbmct::expr<code_printf2t, esbmct::expr2tc_vec_operands>;

class code_expression2t : public esbmct::expr<code_expression2t,
                                              esbmct::expr2tc_operand>
{
public:
  code_expression2t(const expr2tc &oper)
    : esbmct::expr<code_expression2t, esbmct::expr2tc_operand>
      (type_pool.get_empty(), code_expression_id, oper) {}
  code_expression2t(const code_expression2t &ref)
    : esbmct::expr<code_expression2t, esbmct::expr2tc_operand> (ref) {}
};
template class esbmct::expr<code_expression2t, esbmct::expr2tc_operand>;

class code_return2t : public esbmct::expr<code_return2t,
                                          esbmct::expr2tc_operand>
{
public:
  code_return2t(const expr2tc &oper)
    : esbmct::expr<code_return2t, esbmct::expr2tc_operand>
      (type_pool.get_empty(), code_return_id, oper) {}
  code_return2t(const code_return2t &ref)
    : esbmct::expr<code_return2t, esbmct::expr2tc_operand> (ref) {}
};
template class esbmct::expr<code_return2t, esbmct::expr2tc_operand>;

class code_skip2t : public esbmct::expr<code_skip2t>
{
public:
  code_skip2t()
    : esbmct::expr<code_skip2t> (type_pool.get_empty(), code_skip_id) {}
  code_skip2t(const code_skip2t &ref)
    : esbmct::expr<code_skip2t> (ref) {}
};
template class esbmct::expr<code_skip2t>;

class code_free2t : public esbmct::expr<code_free2t,
                                          esbmct::expr2tc_operand>
{
public:
  code_free2t(const expr2tc &oper)
    : esbmct::expr<code_free2t, esbmct::expr2tc_operand>
      (type_pool.get_empty(), code_free_id, oper) {}
  code_free2t(const code_free2t &ref)
    : esbmct::expr<code_free2t, esbmct::expr2tc_operand> (ref) {}
};
template class esbmct::expr<code_free2t, esbmct::expr2tc_operand>;

class code_goto2t : public esbmct::expr<code_goto2t, esbmct::irepidt_target>
{
public:
  code_goto2t(const irep_idt &targ)
    : esbmct::expr<code_goto2t, esbmct::irepidt_target>
      (type_pool.get_empty(), code_goto_id, targ) {}
  code_goto2t(const code_goto2t &ref)
    : esbmct::expr<code_goto2t, esbmct::irepidt_target> (ref) {}
};
template class esbmct::expr<code_goto2t, esbmct::irepidt_target>;

class object_descriptor2t : public esbmct::expr<object_descriptor2t,
                                                esbmct::expr2tc_object,
                                                esbmct::expr2tc_offset>
{
public:
  object_descriptor2t(const type2tc &t, const expr2tc &root,const expr2tc &offs)
    : esbmct::expr<object_descriptor2t, esbmct::expr2tc_object,
                   esbmct::expr2tc_offset>
      (t, object_descriptor_id, root, offs) {}
  object_descriptor2t(const object_descriptor2t &ref)
    : esbmct::expr<object_descriptor2t, esbmct::expr2tc_object,
                   esbmct::expr2tc_offset> (ref) {}
  const expr2tc &get_root_object(void) const;
};
template class esbmct::expr<object_descriptor2t, esbmct::expr2tc_object,
                                                 esbmct::expr2tc_offset>;

class code_function_call2t : public esbmct::expr<code_function_call2t,
                                                 esbmct::expr2tc_ret,
                                                 esbmct::expr2tc_function,
                                                 esbmct::expr2tc_vec_operands>
{
public:
  code_function_call2t(const expr2tc &r, const expr2tc &func,
                       const std::vector<expr2tc> args)
    : esbmct::expr<code_function_call2t, esbmct::expr2tc_ret,
                   esbmct::expr2tc_function, esbmct::expr2tc_vec_operands>
      (type_pool.get_empty(), code_function_call_id, r, func, args) {}
  code_function_call2t(const code_function_call2t &ref)
    : esbmct::expr<code_function_call2t, esbmct::expr2tc_ret,
                   esbmct::expr2tc_function, esbmct::expr2tc_vec_operands>
      (ref) { }
};
template class esbmct::expr<code_function_call2t, esbmct::expr2tc_ret,
                                                  esbmct::expr2tc_function,
                                                  esbmct::expr2tc_vec_operands>;

class code_comma2t : public esbmct::expr<code_comma2t, esbmct::expr2tc_side_1,
                                                       esbmct::expr2tc_side_2>
{
public:
  code_comma2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
    : esbmct::expr<code_comma2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (t, code_comma_id, s1, s2) {}
  code_comma2t(const code_comma2t &ref)
    : esbmct::expr<code_comma2t, esbmct::expr2tc_side_1, esbmct::expr2tc_side_2>
      (ref) { }
};
template class esbmct::expr<code_comma2t, esbmct::expr2tc_side_1,
                                          esbmct::expr2tc_side_2>;

class invalid_pointer2t : public esbmct::expr<invalid_pointer2t,
                                                 esbmct::expr2tc_ptr_obj>
{
public:
  invalid_pointer2t(const expr2tc &obj)
    : esbmct::expr<invalid_pointer2t, esbmct::expr2tc_ptr_obj>
      (type_pool.get_bool(), invalid_pointer_id, obj) {}
  invalid_pointer2t(const invalid_pointer2t &ref)
    : esbmct::expr<invalid_pointer2t, esbmct::expr2tc_ptr_obj>
      (ref) { }
};
template class esbmct::expr<invalid_pointer2t, esbmct::expr2tc_ptr_obj>;

class buffer_size2t : public esbmct::expr<buffer_size2t, esbmct::expr2tc_value>
{
public:
  buffer_size2t(const type2tc &t, const expr2tc &obj)
    : esbmct::expr<buffer_size2t, esbmct::expr2tc_value>
      (t, buffer_size_id, obj) {}
  buffer_size2t(const buffer_size2t &ref)
    : esbmct::expr<buffer_size2t, esbmct::expr2tc_value>
      (ref) { }
};
template class esbmct::expr<buffer_size2t, esbmct::expr2tc_value>;

inline bool operator==(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return !(a == b);
}

inline bool operator<(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(boost::shared_ptr<type2t> const & a, boost::shared_ptr<type2t> const & b)
{
  return (*b.get() < *a.get());
}

inline bool operator==(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() != *b.get());
}

inline bool operator<(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(const expr2tc& a, const expr2tc& b)
{
  return (*b.get() < *a.get());
}

inline std::ostream& operator<<(std::ostream &out, const expr2tc& a)
{
  out << a->pretty(0);
  return out;
}

struct irep2_hash
{
  size_t operator()(const expr2tc &ref) const { return ref->crc(); }
};

struct type2_hash
{
  size_t operator()(const type2tc &ref) const { return ref->crc(); }
};

// Same deal as for "type_macros".
#ifdef NDEBUG
#define dynamic_cast static_cast
#endif
#define expr_macros(name) \
  inline bool is_##name##2t(const expr2tc &t) \
    { return t->expr_id == expr2t::name##_id; } \
  inline bool is_##name##2t(const expr2t &r) \
    { return r.expr_id == expr2t::name##_id; } \
  inline const name##2t & to_##name##2t(const expr2tc &t) \
    { return dynamic_cast<const name##2t &> (*t); } \
  inline name##2t & to_##name##2t(expr2tc &t) \
    { return dynamic_cast<name##2t &> (*t.get()); }

expr_macros(constant_int);
expr_macros(constant_fixedbv);
expr_macros(constant_bool);
expr_macros(constant_string);
expr_macros(constant_struct);
expr_macros(constant_union);
expr_macros(constant_array);
expr_macros(constant_array_of);
expr_macros(symbol);
expr_macros(typecast);
expr_macros(if);
expr_macros(equality);
expr_macros(notequal);
expr_macros(lessthan);
expr_macros(greaterthan);
expr_macros(lessthanequal);
expr_macros(greaterthanequal);
expr_macros(not);
expr_macros(and);
expr_macros(or);
expr_macros(xor);
expr_macros(implies);
expr_macros(bitand);
expr_macros(bitor);
expr_macros(bitxor);
expr_macros(bitnand);
expr_macros(bitnor);
expr_macros(bitnxor);
expr_macros(bitnot);
expr_macros(lshr);
expr_macros(neg);
expr_macros(abs);
expr_macros(add);
expr_macros(sub);
expr_macros(mul);
expr_macros(div);
expr_macros(modulus);
expr_macros(shl);
expr_macros(ashr);
expr_macros(same_object);
expr_macros(pointer_offset);
expr_macros(pointer_object);
expr_macros(address_of);
expr_macros(byte_extract);
expr_macros(byte_update);
expr_macros(with);
expr_macros(member);
expr_macros(index);
expr_macros(zero_string);
expr_macros(zero_length_string);
expr_macros(isnan);
expr_macros(overflow);
expr_macros(overflow_cast);
expr_macros(overflow_neg);
expr_macros(unknown);
expr_macros(invalid);
expr_macros(null_object);
expr_macros(dynamic_object);
expr_macros(dereference);
expr_macros(valid_object);
expr_macros(deallocated_obj);
expr_macros(dynamic_size);
expr_macros(sideeffect);
expr_macros(code_block);
expr_macros(code_assign);
expr_macros(code_init);
expr_macros(code_decl);
expr_macros(code_printf);
expr_macros(code_expression);
expr_macros(code_return);
expr_macros(code_skip);
expr_macros(code_free);
expr_macros(code_goto);
expr_macros(object_descriptor);
expr_macros(code_function_call);
expr_macros(code_comma);
expr_macros(invalid_pointer);
expr_macros(buffer_size);
#undef expr_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

inline bool is_constant_expr(const expr2tc &t)
{
  return t->expr_id == expr2t::constant_int_id ||
         t->expr_id == expr2t::constant_fixedbv_id ||
         t->expr_id == expr2t::constant_bool_id ||
         t->expr_id == expr2t::constant_string_id ||
         t->expr_id == expr2t::constant_struct_id ||
         t->expr_id == expr2t::constant_union_id ||
         t->expr_id == expr2t::constant_array_id ||
         t->expr_id == expr2t::constant_array_of_id;
}

inline bool is_structure_type(const type2tc &t)
{
  return t->type_id == type2t::struct_id || t->type_id == type2t::union_id;
}

inline bool is_nil_expr(const expr2tc &exp)
{
  if (exp.get() == NULL)
    return true;
  return false;
}

inline bool is_nil_type(const type2tc &t)
{
  if (t.get() == NULL)
    return true;
  return false;
}

typedef irep_container<constant_int2t, expr2t::constant_int_id> constant_int2tc;
typedef irep_container<constant_fixedbv2t, expr2t::constant_fixedbv_id>
                       constant_fixedbv2tc;
typedef irep_container<constant_bool2t, expr2t::constant_bool_id>
                       constant_bool2tc;
typedef irep_container<constant_string2t, expr2t::constant_string_id>
                       constant_string2tc;
typedef irep_container<constant_struct2t, expr2t::constant_struct_id>
                       constant_struct2tc;
typedef irep_container<constant_union2t, expr2t::constant_union_id>
                       constant_union2tc;
typedef irep_container<constant_array2t, expr2t::constant_array_id>
                       constant_array2tc;
typedef irep_container<constant_array_of2t, expr2t::constant_array_of_id>
                       constant_array_of2tc;
typedef irep_container<symbol2t, expr2t::symbol_id> symbol2tc;
typedef irep_container<typecast2t, expr2t::typecast_id> typecast2tc;
typedef irep_container<if2t, expr2t::if_id> if2tc;
typedef irep_container<equality2t, expr2t::equality_id> equality2tc;
typedef irep_container<notequal2t, expr2t::notequal_id> notequal2tc;
typedef irep_container<lessthan2t, expr2t::lessthan_id> lessthan2tc;
typedef irep_container<greaterthan2t, expr2t::greaterthan_id> greaterthan2tc;
typedef irep_container<lessthanequal2t, expr2t::lessthanequal_id>
                       lessthanequal2tc;
typedef irep_container<greaterthanequal2t, expr2t::greaterthanequal_id>
                       greaterthanequal2tc;
typedef irep_container<not2t, expr2t::not_id> not2tc;
typedef irep_container<and2t, expr2t::and_id> and2tc;
typedef irep_container<or2t, expr2t::or_id> or2tc;
typedef irep_container<xor2t, expr2t::xor_id> xor2tc;
typedef irep_container<implies2t, expr2t::implies_id> implies2tc;
typedef irep_container<bitand2t, expr2t::bitand_id> bitand2tc;
typedef irep_container<bitor2t, expr2t::bitor_id> bitor2tc;
typedef irep_container<bitxor2t, expr2t::bitxor_id> bitxor2tc;
typedef irep_container<bitnand2t, expr2t::bitnand_id> bitnand2tc;
typedef irep_container<bitnor2t, expr2t::bitnor_id> bitnor2tc;
typedef irep_container<bitnxor2t, expr2t::bitnxor_id> bitnxor2tc;
typedef irep_container<bitnot2t, expr2t::bitnot_id> bitnot2tc;
typedef irep_container<lshr2t, expr2t::lshr_id> lshr2tc;
typedef irep_container<neg2t, expr2t::neg_id> neg2tc;
typedef irep_container<abs2t, expr2t::abs_id> abs2tc;
typedef irep_container<add2t, expr2t::add_id> add2tc;
typedef irep_container<sub2t, expr2t::sub_id> sub2tc;
typedef irep_container<mul2t, expr2t::mul_id> mul2tc;
typedef irep_container<div2t, expr2t::div_id> div2tc;
typedef irep_container<modulus2t, expr2t::modulus_id> modulus2tc;
typedef irep_container<shl2t, expr2t::shl_id> shl2tc;
typedef irep_container<ashr2t, expr2t::ashr_id> ashr2tc;
typedef irep_container<same_object2t, expr2t::same_object_id> same_object2tc;
typedef irep_container<pointer_offset2t, expr2t::pointer_offset_id>
                       pointer_offset2tc;
typedef irep_container<pointer_object2t, expr2t::pointer_object_id>
                       pointer_object2tc;
typedef irep_container<address_of2t, expr2t::address_of_id> address_of2tc;
typedef irep_container<byte_extract2t, expr2t::byte_extract_id> byte_extract2tc;
typedef irep_container<byte_update2t, expr2t::byte_update_id> byte_update2tc;
typedef irep_container<with2t, expr2t::with_id> with2tc;
typedef irep_container<member2t, expr2t::member_id> member2tc;
typedef irep_container<index2t, expr2t::index_id> index2tc;
typedef irep_container<zero_string2t, expr2t::zero_string_id> zero_string2tc;
typedef irep_container<zero_length_string2t, expr2t::zero_length_string_id>
                       zero_length_string2tc;
typedef irep_container<isnan2t, expr2t::isnan_id> isnan2tc;
typedef irep_container<overflow2t, expr2t::overflow_id> overflow2tc;
typedef irep_container<overflow_cast2t, expr2t::overflow_cast_id>
                       overflow_cast2tc;
typedef irep_container<overflow_neg2t, expr2t::overflow_neg_id>overflow_neg2tc;
typedef irep_container<unknown2t, expr2t::unknown_id> unknown2tc;
typedef irep_container<invalid2t, expr2t::invalid_id> invalid2tc;
typedef irep_container<dynamic_object2t, expr2t::dynamic_object_id>
                       dynamic_object2tc;
typedef irep_container<dereference2t, expr2t::dereference_id> dereference2tc;
typedef irep_container<valid_object2t, expr2t::valid_object_id> vaild_object2tc;
typedef irep_container<deallocated_obj2t, expr2t::deallocated_obj_id>
                       deallocated_obj2tc;
typedef irep_container<dynamic_size2t, expr2t::dynamic_size_id> dynamic_size2tc;
typedef irep_container<sideeffect2t, expr2t::sideeffect_id> sideeffect2tc;
typedef irep_container<code_block2t, expr2t::code_block_id> code_block2tc;
typedef irep_container<code_assign2t, expr2t::code_assign_id> code_assign2tc;
typedef irep_container<code_init2t, expr2t::code_init_id> code_init2tc;
typedef irep_container<code_decl2t, expr2t::code_decl_id> code_decl2tc;
typedef irep_container<code_printf2t, expr2t::code_printf_id> code_printf2tc;
typedef irep_container<code_expression2t, expr2t::code_expression_id>
                       code_expression2tc;
typedef irep_container<code_return2t, expr2t::code_return_id> code_return2tc;
typedef irep_container<code_skip2t, expr2t::code_skip_id> code_skip2tc;
typedef irep_container<code_free2t, expr2t::code_free_id> code_free2tc;
typedef irep_container<code_goto2t, expr2t::code_goto_id> code_goto2tc;
typedef irep_container<object_descriptor2t, expr2t::object_descriptor_id>
                       object_descriptor2tc;
typedef irep_container<code_function_call2t, expr2t::code_function_call_id>
                       code_function_call2tc;
typedef irep_container<code_comma2t, expr2t::code_comma_id> code_comma2tc;
typedef irep_container<invalid_pointer2t, expr2t::invalid_pointer_id>
                       code_invalid_pointer2tc;
typedef irep_container<buffer_size2t, expr2t::buffer_size_id> buffer_size2tc;

// XXXjmorse - to be moved into struct union superclass when it exists.
inline unsigned int
get_component_number(const type2tc &type, const irep_idt &name)
{
  const std::vector<irep_idt> &member_names = (is_struct_type(type))
    ? to_struct_type(type).member_names : to_union_type(type).member_names;

  unsigned int i = 0;
  forall_names(it, member_names) {
    if (*it == name)
      return i;
    i++;
  }

  assert(0);
}

#endif /* _UTIL_IREP2_H_ */
