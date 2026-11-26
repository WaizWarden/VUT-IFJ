// Vzajemna rekurze a volani funkce pred jeji definici.
import ifj26.*

def isOdd(n: Int): Int =
  if n == 0 then
    return 0
  val m = n - 1
  val r = isEven(m)
  return r

def isEven(n: Int): Int =
  if n == 0 then
    return 1
  val m = n - 1
  val r = isOdd(m)
  return r

@main def mutualRecursionTest(): Unit =
  val a = isEven(10)
  val b = isOdd(10)
  val c = isEven(7)
  val d = isOdd(7)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
  print(d)
  print("\n")
