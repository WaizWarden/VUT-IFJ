// Vicenradkovy string literal: zadne escape sekvence se nevyhodnocuji.
import ifj26.*

@main def multilineStringsTest(): Unit =
  val x = """first
second\nthird
"""
  print(x)
  print("end\n")
