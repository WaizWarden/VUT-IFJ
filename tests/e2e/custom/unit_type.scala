// Typ Unit, literal (), funkce bez navratove hodnoty.
import ifj26.*

def doNothing(): Unit =
  print("in doNothing\n")

def doNothingExpr(): Unit = ()

@main def unitTypeTest(): Unit =
  doNothing()
  doNothingExpr()
  print("done\n")
