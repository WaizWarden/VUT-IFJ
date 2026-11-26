// VALMEM: retezeni clenu, length spocteneho retezce, toString cisla.
import ifj26.*

@main def valmemChainTest(): Unit =
  val len = "hello".length
  print(len)
  print("\n")
  val word = "ab" + "cd"
  val wlen = word.length
  print(wlen)
  print("\n")
  val digits = 12345.toString
  val dlen = digits.length
  print(dlen)
  print("\n")
  val neg = 0 - 17
  val s = neg.toString
  print(s)
  print("\n")
