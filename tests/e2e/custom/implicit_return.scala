// Implicitni navrat poslednim vyrazem, jednoexpressionova tela.
import ifj26.*

def add(a: Int, b: Int): Int =
  a + b

def square(n: Int): Int = n * n

def absVal(n: Int): Int =
  if n < 0 then
    return 0 - n
  n

@main def implicitReturnTest(): Unit =
  val x = add(3, 4)
  val y = square(5)
  val neg = 0 - 7
  val z = absVal(neg)
  val w = absVal(9)
  print(x)
  print("\n")
  print(y)
  print("\n")
  print(z)
  print("\n")
  print(w)
  print("\n")
