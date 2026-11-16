import ifj26.*

def square(n: Int): Int =
  n * n

@main def run(): Unit =
  val n = readInt()
  val result = square(n)
  print("square: ")
  print(result)
  print("\n")