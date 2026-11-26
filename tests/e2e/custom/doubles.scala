// Aritmetika Double, implicitni konverze Int -> Double, toInt, toStr.
import ifj26.*

@main def doublesTest(): Unit =
  val a = 10.0 / 4.0
  val sa = toStr(a)
  print(sa)
  print("\n")
  val i = 3
  val b = 2.5 * i
  val sb = toStr(b)
  print(sb)
  print("\n")
  val c = toInt(a)
  print(c)
  print("\n")
  if a > 2.0 then
    print("gt\n")
  else
    print("le\n")
  val d = 1.5 + 1.5
  val di = toInt(d)
  print(di)
  print("\n")
