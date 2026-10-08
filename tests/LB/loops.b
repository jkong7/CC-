int64 collatz (int64 n) {
  int64 steps, half, rem
  steps <- 0
  while (n > 1) :step :done
  {
    :step
    half <- n >> 1
    rem <- n & 1
    if (rem = 0) :even :odd
    :even
    n <- half
    steps <- steps + 1
    continue
    :odd
    n <- n * 3
    n <- n + 1
    steps <- steps + 1
    continue
  }
  :done
  return steps
}

void main () {
  int64 i, total, c
  i <- 1
  total <- 0
  while (i <= 10) :body :exit
  {
    :body
    c <- collatz(i)
    print(c)
    total <- total + c
    i <- i + 1
    if (total > 60) :stop :again
    :stop
    break
    :again
    continue
  }
  :exit
  print(total)
  print(i)
}
