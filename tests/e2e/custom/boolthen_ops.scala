// BOOLTHEN: literaly true/false, negace, &&, ||, zkracene vyhodnocovani.
import ifj26.*

@main def boolthenOpsTest(): Unit =
  val t = true
  val f = false
  if t then
    print("t\n")
  else
    print("f\n")
  if !f then
    print("notf\n")
  else
    print("f\n")
  if t && f then
    print("and\n")
  else
    print("notand\n")
  if t || f then
    print("or\n")
  else
    print("notor\n")
  if t == true then
    print("eq\n")
  else
    print("neq\n")
