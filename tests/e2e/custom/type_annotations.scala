// Explicitni typove anotace promennych a funkci.
import ifj26.*

def typed(a: Int, b: Double): Double =
  val x: Double = a
  val y: Double = b
  x + y

@main def typeAnnotationsTest(): Unit =
  var s: String = "hi"
  val u: Unit = ()
  val r = typed(1, 2.5)
  val rs = toStr(r)
  print(s)
  print("\n")
  print(rs)
  print("\n")
