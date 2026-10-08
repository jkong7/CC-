void main () {
  int64 x
  x <- 1
  print(x)
  {
    int64 x
    x <- 2
    print(x)
    {
      int64 x, y
      x <- 3
      y <- x * 10
      print(y)
    }
    print(x)
  }
  print(x)
  {
    int64 y
    y <- x + 100
    print(y)
  }
}
