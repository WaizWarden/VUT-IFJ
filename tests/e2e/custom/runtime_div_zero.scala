// Celociselne deleni nulou je behova chyba 57.
import ifj26.*

@main def runtimeDivZeroTest(): Unit =
  val zero = readInt()
  val x = 10 / zero
  print(x)
