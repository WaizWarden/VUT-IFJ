// Specifikace: blokove komentare mohou byt vnorene; komentar bez noveho radku
// se chova jako bily znak, s novym radkem jako konec radku.
import ifj26.*

@main def blockCommentsNestedTest(): Unit =
  /* komentar /* vnoreny */ stale komentar */
  print("a\n")
  /* vice
     radku
  */
  print("b\n")
  /* komentar sam na radku */
  print("c\n")
