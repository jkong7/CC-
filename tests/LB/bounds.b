void main () {
  int64[] a
  int64 i, v
  a <- new Array(3)
  i <- 0
  while (i < 5) :body :done
  {
    :body
    v <- a[i]
    i <- i + 1
    continue
  }
  :done
}
