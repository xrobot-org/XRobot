"""Reading class definitions from Module headers (xrobot.type_index)."""
import unittest

from fixtures import TempDirTestCase

from xrobot.type_index import TypeIndex

FIELDS = '''#pragma once
#include <functional>
namespace ns {
struct Fields {
  alignas(8) int aligned;
  int bits : 3;
  inline static int shared = 0;
  static constexpr int kMax = 4;
  static int Count();
  int a = 1, b{2};
  int arr[3];
  int* ptr = nullptr;
  mutable int cache;
  std::function<int()> fn = [] { int k = 0; return k; };
  int computed = [] { return 1; }();
  void Method() { int local = 0; (void)local; }
  int Get() const { return a; }
  using Alias = int;
  Alias alias_field;
  struct Inner { int y; } inner;
  enum class Mode { A, B } mode = Mode::A;
  [[maybe_unused]] int attributed;
  int paren = int(4);
  friend struct Other;
};
}  // namespace ns
'''


class TypeIndexTestCase(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp

    def index(self, *texts):
        paths = [self.write(f'H{i}.hpp', text) for i, text in enumerate(texts)]
        return TypeIndex(paths)


class Fields(TypeIndexTestCase):
    def test_data_members_are_read_in_declaration_order(self):
        entry = self.index(FIELDS).resolve('ns::Fields')
        self.assertEqual([name for name, _, _ in entry.fields()],
                         ['aligned', 'bits', 'a', 'b', 'arr', 'ptr', 'cache', 'fn', 'computed', 'alias_field',
                          'inner', 'mode', 'attributed', 'paren'])
        types = {name: cpp_type for name, cpp_type, _ in entry.fields()}
        self.assertEqual((types['aligned'], types['ptr'], types['fn'], types['inner'], types['mode']),
                         ('int', 'int*', 'std::function<int()>', 'Inner', 'Mode'))

    def test_default_member_initializer_texts(self):
        defaults = self.index(FIELDS).resolve('ns::Fields').layout().field_defaults
        self.assertEqual(defaults, {'a': '1', 'b': '{2}', 'ptr': 'nullptr', 'fn': '[] { int k = 0; return k; }',
                                    'computed': '[] { return 1; }()', 'mode': 'Mode::A', 'paren': 'int(4)'})

    def test_bit_field_default_member_initializer(self):
        entry = self.index('struct Bits { unsigned flag : 1 = 1; unsigned mode : 3 {2}; };').resolve('Bits')
        self.assertEqual(entry.layout().field_defaults, {'flag': '1', 'mode': '{2}'})

    def test_constructor_bodies_and_initializer_lists_are_not_fields(self):
        entry = self.index('class WithCtor {\n public:\n  WithCtor() : a_(1), b_{2} { int z = 3; (void)z; }\n'
                           '  explicit WithCtor(int v) : a_(v) {}\n private:\n  int a_; int b_;\n};\n').resolve('WithCtor')
        self.assertEqual(entry.fields(), [('a_', 'int', 'private'), ('b_', 'int', 'private')])
        self.assertEqual([[p['name'] for p in c] for c in entry.constructors()], [[], ['v']])
        self.assertFalse(entry.is_aggregate())

    def test_typedef_struct_at_namespace_scope_is_indexed(self):
        entry = self.index('namespace ns { typedef struct { int p; float q = 1.0f; } Plain; }').resolve('ns::Plain')
        self.assertIsNotNone(entry)
        self.assertEqual(entry.fields(), [('p', 'int', 'public'), ('q', 'float', 'public')])
        self.assertEqual(entry.layout().field_defaults, {'q': '1.0f'})
        self.assertTrue(entry.is_aggregate())

    def test_attributed_class_name_is_indexed(self):
        entry = self.index('struct [[nodiscard]] Attr { int a; };').resolve('Attr')
        self.assertIsNotNone(entry)
        self.assertEqual(entry.qualified, 'Attr')


class Aggregates(TypeIndexTestCase):
    def test_aggregate_and_mapping_problems(self):
        index = self.index('''
struct Plain { int a; };
struct Pure { virtual void F() = 0; int x; };
struct Private { private: int hidden; };
struct Derived : Plain { int b; };
struct Ctor { Ctor(int a) {} int a; };
struct WithUnion { union { int i; float f; } u; };
struct Conditional { int a;
#ifdef FEATURE
  int b;
#endif
};''')
        self.assertTrue(index.resolve('Plain').is_aggregate())
        for name in ('Pure', 'Private', 'Derived', 'Ctor'):
            with self.subTest(name=name):
                self.assertFalse(index.resolve(name).is_aggregate())
        self.assertIsNone(index.resolve('Plain').mapping_problem())
        self.assertIsNone(index.resolve('Ctor').mapping_problem())
        self.assertIn('contains a union', index.resolve('WithUnion').mapping_problem())
        self.assertIn('declares fields under #if (b)', index.resolve('Conditional').mapping_problem())
        self.assertIn('base classes or virtual functions and no constructor', index.resolve('Derived').mapping_problem())
        self.assertIn('base classes or virtual functions and no constructor', index.resolve('Pure').mapping_problem())


class Lookup(TypeIndexTestCase):
    def test_names_resolve_outward_from_a_scope_and_through_aliases(self):
        index = self.index(FIELDS + 'namespace ns { using Options = Fields; struct Use { Options o; }; }\n'
                                    'struct Fields { int global; };\n')
        self.assertEqual(index.resolve('Options', ('ns', 'Use')).qualified, 'ns::Fields')
        self.assertEqual(index.resolve('ns::Options').qualified, 'ns::Fields')
        self.assertEqual(index.resolve('Fields', ('ns', 'Use')).qualified, 'ns::Fields')
        self.assertEqual(index.resolve('Fields').qualified, 'Fields')
        self.assertEqual(index.resolve('const ns::Fields&').qualified, 'ns::Fields')
        self.assertIsNone(index.resolve('ns::Fields*'))
        self.assertIsNone(index.resolve('Missing'))

    def test_enclosing_template_parameters_are_not_resolved_to_outer_types(self):
        index = self.index('struct T { int x; };\ntemplate <class T> class Outer {\n public:\n'
                           '  struct P { T value; int n; };\n};\n')
        self.assertIsNone(index.resolve('T', ('Outer', 'P')))
        self.assertEqual(index.resolve('T').qualified, 'T')

    def test_unknown_bases_make_member_lookups_unknown(self):
        index = self.index('struct Inner { int x; };\nstruct Known { struct Inner2 { int y; }; };\n'
                           'struct Holder : Vendor::Base { struct Q { Inner value; }; };\n'
                           'struct Child : Known { struct R { Inner2 value; }; };\n')
        self.assertIsNone(index.resolve('Inner', ('Holder', 'Q')))
        self.assertEqual(index.resolve('Inner2', ('Child', 'R')).qualified, 'Known::Inner2')

    def test_qualify_in_substitutes_enclosing_template_arguments(self):
        index = self.index('template <class T, int N>\nclass Outer {\n public:\n  enum class Mode { A };\n'
                           '  struct P { T value; Mode mode; int n = N; };\n};\n')
        entry = index.resolve('Outer::P')
        self.assertEqual(index.qualify_in('T', entry, 'Outer<Layout, 4>::P'), 'Layout')
        self.assertEqual(index.qualify_in('Mode', entry, 'Outer<Layout, 4>::P'), 'Outer<Layout, 4>::Mode')
        self.assertEqual(index.qualify_in('std::array<T, N>', entry, 'Outer<Layout, 4>::P'), 'std::array<Layout, 4>')

    def test_qualify_in_names_member_and_namespace_types(self):
        index = self.index(FIELDS)
        entry = index.resolve('ns::Fields')
        self.assertEqual(index.qualify_in('Mode', entry, 'ns::Fields'), 'ns::Fields::Mode')
        self.assertEqual(index.qualify_in('Alias', entry, 'ns::Fields'), 'ns::Fields::Alias')
        self.assertEqual(index.qualify_in('int', entry, 'ns::Fields'), 'int')
        self.assertEqual(index.qualify_in('other::Mode', entry, 'ns::Fields'), 'other::Mode')

    def test_a_type_defined_in_two_headers_is_an_error(self):
        index = self.index('struct Twice { int a; };', 'struct Twice { int b; };')
        with self.assertRaisesRegex(ValueError, 'Type Twice is defined in several Module headers'):
            index.resolve('Twice')

    def test_global_class_names(self):
        index = self.index('struct A {};\ntemplate <typename T>\nclass B {};\nclass alignas(8) C {};\n'
                           'namespace n {\nstruct D {};\n}\nvoid f() { struct Local {}; }\n')
        self.assertTrue({'A', 'B', 'C'} <= index.global_class_names())
        self.assertNotIn('Local', index.global_class_names())


class Monitor(TypeIndexTestCase):
    def provides(self, text, name='M'):
        index = self.index(text)
        return index.provides_monitor(index.resolve(name))

    def test_public_monitor(self):
        self.assertTrue(self.provides('class M { public: void OnMonitor(); };'))
        self.assertTrue(self.provides('struct M { void OnMonitor() const noexcept {} };'))
        self.assertFalse(self.provides('class M { void OnMonitor(); };'))
        self.assertFalse(self.provides('class M { protected: void OnMonitor(); };'))
        self.assertFalse(self.provides('class M { public: void OnMonitorAll(); };'))

    def test_public_static_monitor(self):
        self.assertTrue(self.provides('class M { public: static void OnMonitor(); };'))

    def test_inherited_and_using_declared_monitors(self):
        self.assertTrue(self.provides('struct B { void OnMonitor(); };\nclass M : public B {};'))
        self.assertFalse(self.provides('struct B { void OnMonitor(); };\nclass M : B {};'))
        self.assertFalse(self.provides('struct B { void OnMonitor(); };\nclass M : protected B {};'))
        self.assertTrue(self.provides('class B { protected: void OnMonitor(); };\n'
                                      'class M : B { public: using B::OnMonitor; };'))

    def test_libxr_bases_provide_none_and_unknown_bases_are_unknown(self):
        self.assertFalse(self.provides('class M : public LibXR::Application { public: M(); };'))
        self.assertFalse(self.provides('class M : public ::LibXR::Application { public: M(); };'))
        self.assertIsNone(self.provides('class M : public Vendor::Base { public: M(); };'))
        self.assertTrue(self.provides('class M : public Vendor::Base { public: void OnMonitor(); };'))

    def test_conditional_monitor_is_an_error(self):
        for text in ('class M { public:\n#if FEATURE\n  void OnMonitor();\n#endif\n};',
                     'struct B { void X(); };\nclass M : public B { public:\n#ifdef FEATURE\n  using B::OnMonitor;\n#endif\n};'):
            with self.subTest(text=text), self.assertRaisesRegex(ValueError, 'M declares OnMonitor under #if'):
                self.provides(text)


if __name__ == '__main__':
    unittest.main()
