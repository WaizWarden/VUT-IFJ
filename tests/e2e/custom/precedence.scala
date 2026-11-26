// Priorita a asociativita operatoru, unarni minus, zavorky.
import ifj26.*

@main def precedenceTest(): Unit =
  val a = 2 + 3 * 4
  val b = (2 + 3) * 4
  val c = 10 - 4 - 3
  val d = 20 / 5 / 2
  val e = 2 * 3 + 4 * 5
  val f = 0 - 2 + 5
  val g = 100 - 10 * 5 - 25
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
  print(d)
  print("\n")
  print(e)
  print("\n")
  print(f)
  print("\n")
  print(g)
  print("\n")
