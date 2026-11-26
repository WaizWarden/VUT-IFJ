// Vnorenecyklus while, soucet nasobici tabulky.
import ifj26.*

@main def whileNestedTest(): Unit =
  var sum = 0
  var i = 1
  while i <= 3 do
    var j = 1
    while j <= 3 do
      sum = sum + i * j
      j = j + 1
    i = i + 1
  print(sum)
  print("\n")
