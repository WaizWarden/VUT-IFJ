// chr mimo interval 0 az 127 konci behovou chybou 58.
import ifj26.*

@main def runtimeChrTest(): Unit =
  val c = chr(200)
  print(c)
