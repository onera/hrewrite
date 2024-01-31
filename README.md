
# The hrewrite Library


**hrewrite** is a pure C++ header library for term rewriting, with a python3 interface.
This library has the following features:
 - rewriting of unranked trees, based on [[1]](#1)
 - guards in conditional rewriting rules can be arbitrary python functions
 - terms can store python objects
 - it can be configured so each term correspond to a unique object in memory (a term is modeled as a Directed Acyclic Graph), and its normal form is computed at most once.

### Installation

This library is implemented in C++17 with optional python3 bindings, and the building process is done using cmake and bash.
The following lines clone the hrewrite repository, compile the library, generate the python bindings locally and update the `PYTHONPATH` variable:
```bash
$ git clone https://github.com/onera/hrewrite.git
$ cd hrewrite
$ source build.sh
```


### An Example of the PYTHON API

To use our library, we first need to import it:
```python
import hrewrite as hrw
```

#### Simple Natural Numbers

We start by declaring three sorts (
 one for the zero value,
 one for the non-zero values,
 and one for all natural integers) with the corresponding subsort relation
```python
hrw.sorts("Zero", "NzNat", "Nat")
hrw.subsort("Zero", "Nat")
hrw.subsort("NzNat", "Nat")
```
The name of a sort can be any string.
Next, we declare the following term constructors:
```python
zero = hrw.constructor("zero", hrw.free() >> "Zero")
s = hrw.constructor("s", hrw.free("Nat") >> "NzNat")
```
The first line declares the constructor named `"zero"`, that takes no parameters and is of the sort `"Zero"`, and stores it in the variable `zero`.
The second line declares the constructor named `"s"`, that takes one parameter of sort `"Nat"` and returns a term of sort `"NzNat"`,  and stores it in the variable `s`.
Term can be constructed and printed as follows:
```python
two = s(s(zero))
print(two)
```

Finally, we declare the `plus` function and its semantics as rewriting rules:
```python
plus = hrw.constructor("plus", hrw.free("Nat Nat") >> "Nat")
alpha, beta = hrw.vars("Nat", "Nat")
rw_eng = hrw.rw_engine_cls()
rw_eng.add(plus(zero, alpha), alpha)
rw_eng.add(plus(s(alpha), beta), plus(alpha, s(beta)))
```
The first line declares the constructor named `"plus"`, that takes two parameters of sort `"Nat"` and returns a term of sort `"Nat"`, and stores it in the variable `plus`.
The second line declares the variables `alpha` and `beta`, both of sort `"Nat"`.
The third line declares a new rewiting context `rw_eng`,
 and the last lines introduce the standard rewriting rules for the semantics of `plus`.
Applying these rules on the term `2+3` is done as follows:
```python
five = rw_eng.rewrite(plus(two, s(two)))
```

#### Rewriting Natural Numbers to Python

To store python number in our terms, we first create a new sort (with the corresponding subsort relation) and a new constructor for this container:
```python
hrw.sort("Val")
hrw.subsort("Val", "Nat")
val = hrw.constructor("val", hrw.lit() >> "Val")
```
In the third line, `hrw.lit()` states that the term constructor `val` holds a python value.
We model the conversion from pure term naturals to python naturals as follows:
```python
valpha, vbeta, vgamma = hrw.vars("Val", "Val", "Val")
# conversion of 0
rw_eng.add(zero, val(0))
# conversion of s
def guard_s(rw_eng, substitution):
  t = hrw.instantiate(valpha, substitution)
  substitution.add(vbeta, val(hrw.get_value(t) + 1))
  return True
rw_eng.add(s(valpha), vbeta, guard_s)
# conversion of plus
def guard_plus(rw_eng, substitution):
  t1 = hrw.instantiate(valpha, substitution)
  t2 = hrw.instantiate(vbeta, substitution)
  substitution.add(vgamma, val(hrw.get_value(t1) + hrw.get_value(t2)))
  return True
rw_eng.add(plus(valpha, vbeta), vgamma, guard_plus)
```
The first rewriting rule (in line 3) simply states that the term `zero` corresponds to `0`.
The second rewriting rule (in line 9)} encodes the semantics of `s`:
 the increment of the integer value is performed within the guard `guard_s`.
The rewriting rule states that the successor of a python value is rewritten to a variable `vbeta`,
 whose image is set in `guard_s`.
A guard (in line 5) is a python function that takes two parameters:
 the rewriting engine executing the guard (in case some rewriting must be performed in the guard),
 and the substitution computed by the pattern matching.
In line 6, the guard extracts the image `t` of `valpha` from the substitution,
 and in line 7, it sets the value of `vbeta` to be a `val` containing the value contained in `t` plus 1.
Finally, in line 8, the function returns `True` to signal the rewriting engine that the guard has been validated.

The last rewriting rule gives the semantics of `plus` on two python numbers, and its implementation is similar to the one for the semantics of `s`.

**Note** that by default, the *hrewrite* python library stores the normal form of a term.
Hence, if one adds new rewriting rules after having computed the normal form of some terms,
 these normal forms may not be correct anymore.
This is typically the case here where we computed `five`.
To clear the registry of normal form of `rw_eng`, we need to call
```python
rw_eng.clear_nf()
```
Moreover, to also clear the set of registered rewriting rules, we call
```python
rw_eng.clear()
```

#### Lists of Natural Numbers
Since *hrewrite* manages unranked trees, the constructor for list of natural numbers can be declared as follows:
```python
hrw.sorts("List")
tlist = hrw.constructor("list", hrw.free("Nat*") >> "List")
```
As indicated by the star in `"Nat*"` in line 2, the `List` constructor can take any number of natural numbers in parameter.
The head and tail of a list can be expressed as follows:
```python
head = hrw.constructor("head", hrw.free("List") >> "Nat")
tail = hrw.constructor("tail", hrw.free("List") >> "List")
lalpha, lbeta = hrw.vars("Nat", "Nat*")
rw_eng.add(head(tlist(lalpha, lbeta)), lalpha)
rw_eng.add(tail(tlist(lalpha, lbeta)), tlist(beta))
```
The first two lines declare the `head` and `tail` term constructors.
The third line declares the variable `lapha` that matches one natural number, and `lbeta` that can match any number of natural numbers.
Line 4 gives the semantics of `head`: the rewriting rule matches `lalpha` with the first number of the list and `lbeta` with the other numbers,
 and simply returns `lalpha`.
Line 5 gives the semantics of `tail`: the pattern of the rule is the same of the previous one, but the image is now the list containing all the numbers in `lbeta`.

The concatenation of two lists is also relatively simple to express:
```python
concat = hrw.constructor("concat", hrw.free("List List") >> "List")
lgamma = hrw.var("Nat*")
rw_eng.add(concat(tlist(lbeta), tlist(lgamma)), tlist(lbeta, lgamma))
```
The rewriting rule in line 3 simply puts the content of the first list in `lbeta`, the content of the second list in `lgamma`,
 and then puts these two contents in sequence in the resulting list.

#### XML terms

This example is taken from [[2]](#2) which presents a `ParentBook` describing XML documents that store information of persons.
In this example we have two kind of persons, `female` and `male`, and a `ParentBook` is a list of such persons:
```python
hrw.sorts("FPerson", "MPerson", "ParentBook")
parentbook = hrw.constructor("parentbook", hrw.free("(FPerson | MPerson)*") >> "ParentBook")
```
Then a person, independently to the gender, has a name, a list of children, and an arbitrary number of phone numbers and emil addresses:
```python
hrw.sorts("Name", "Children", "Tel", "Email", "Kind", "Number")
name, children, tel, email, home, work, number, fperson, mperson = hrw.constructors(
  # content of a person
  name = hrw.lit() >> "Name",
  children = hrw.free("(FPerson | MPerson)*") >> "Children",
  tel = hrw.free("Kind? Number") >> "Tel",
  email = hrw.lit() >> "Email",
  # tel content
  home = hrw.free() >> "Kind",
  work = hrw.free() >> "Kind",
  number = hrw.lit() >> "Number",
  # persons
  fperson = hrw.free("Name Children (Tel | Email)*") >> "FPerson",
  mperson = hrw.free("Name Children (Tel | Email)*") >> "MPerson",
)
```
Here, we first declare the sorts for the different elements construction a person,
 and then use the function `hrw.constructors` to declare several term constructors in on call:
 `name` contains a string, and so we decaled it as a literal;
 `children` simply wraps a list of persons;
 `tel` has a mandatory number and an optional kind;
 `email` contains a string and is declared as a literal;
 `home` and `work` are the two possible kinds of a phone number;
 `number` is a literal;
 and `fperson` and `mperson` are declared as described.

The example of parentbook presented in [[2]](#2) can be written in *hrewrite* as follows:
```python
parents = parentbook(
  fperson(
    name("Clara"),
    children(
      mperson(name("Pål André"), children())
    ),
    email("clara@lri.fr"),
    tel(number("314-1592654")),
  ),
  mperson(
    name("Bob"),
    children(
      fperson(name("Alice"), children()),
      mperson(name("Anne"), children()),
      mperson(name("gender"), children()),
    ),
    tel(work, number("271828")),
    tel(home, number("66260")),
  )
)
```


## References

<a name="1">[1]</a>
Temur Kutsia, and Mircea Marin.
2015. Regular expression order-sorted unification and matching.
In Journal of Symbolic Computation (Vol. 67), 42-67.
*doi: 10.1016/J.JSC.2014.08.002*

<a name="2">[2]</a>
Véronique Benzaken, Giuseppe Castagna, and Alain Frisch.
2003. CDuce: an XML-centric general-purpose language.
In ICFP'03. ACM, 51-63.
*doi: 10.1145/944705.944711*

