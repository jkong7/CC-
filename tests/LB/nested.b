int64[][] table (int64 n) {
  int64[][] t
  int64 i, j, v
  t <- new Array(n, n)
  i <- 0
  while (i < n) :row :rows_done
  {
    :row
    j <- 0
    while (j < n) :col :cols_done
    {
      :col
      if (j > i) :skip :fill
      :fill
      v <- i * j
      t[i][j] <- v
      :skip
      j <- j + 1
      continue
    }
    :cols_done
    i <- i + 1
    continue
  }
  :rows_done
  return t
}

void main () {
  int64[][] t
  int64 i, j, sum, v
  t <- table(5)
  sum <- 0
  i <- 0
  while (i < 5) :outer :outer_done
  {
    :outer
    j <- 0
    while (j < 5) :inner :inner_done
    {
      :inner
      v <- t[i][j]
      if (v = 0) :next :add
      :add
      sum <- sum + v
      if (sum > 50) :bail :next
      :bail
      break
      :next
      j <- j + 1
      continue
    }
    :inner_done
    i <- i + 1
    continue
  }
  :outer_done
  print(sum)
  goto :end
  print(t)
  :end
  print(t)
}
