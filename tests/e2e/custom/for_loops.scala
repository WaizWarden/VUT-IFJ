// CONTROL: cyklus for - inkluzivni/exkluzivni rozsah, krok, straze.
import ifj26.*

@main def forLoopsTest(): Unit =
  var sum = 0
  for i <- 1 to 5 do
    sum = sum + i
  print(sum)
  print("\n")
  sum = 0
  for i <- 1 until 5 do
    sum = sum + i
  print(sum)
  print("\n")
  sum = 0
  for i <- 1 to 10 by 3 do
    sum = sum + i
  print(sum)
  print("\n")
  sum = 0
  for i <- 6 to 2 by -2 do
    sum = sum + i
  print(sum)
  print("\n")
  sum = 0
  for i <- 1 to 10 if i != 4 do
    sum = sum + i
  print(sum)
  print("\n")
