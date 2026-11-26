// Vetveni else if a vnorene podminky.
import ifj26.*

def classify(n: Int): String =
  if n < 0 then
    return "negative"
  else
    if n == 0 then
      return "zero"
    else
      if n < 10 then
        return "small"
      else
        return "big"

@main def ifChainsTest(): Unit =
  val neg = 0 - 5
  val a = classify(neg)
  val b = classify(0)
  val c = classify(5)
  val d = classify(42)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
  print(d)
  print("\n")
