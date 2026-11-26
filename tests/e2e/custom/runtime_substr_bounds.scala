// subStr mimo rozsah konci behovou chybou 40.
import ifj26.*

@main def runtimeSubstrTest(): Unit =
  val s = "abc"
  val x = subStr(s, 1, 10)
  print(x)
