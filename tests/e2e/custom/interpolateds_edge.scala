// INTERPOLATEDS: vice vlozenych hodnot, vnorene volani ve vyrazu, retezeni.
import ifj26.*

@main def interpolatedsEdgeTest(): Unit =
  val a = 3
  val b = 4
  print(s"${a} + ${b} = ${a + b}")
  print("\n")
  val name = "world"
  val greeting = s"hello $name" + "!"
  print(greeting)
  print("\n")
  print(s"len=${name.length}")
  print("\n")
