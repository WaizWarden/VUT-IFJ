// Hluboka rekurze.
import ifj26.*

def fib(n: Int): Int =
  if n <= 1 then
    return n
  val a = n - 1
  val b = n - 2
  val fa = fib(a)
  val fb = fib(b)
  return fa + fb

@main def fibonacciTest(): Unit =
  val r = fib(15)
  print(r)
  print("\n")
