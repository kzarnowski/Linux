#include <stdio.h>


int main() {
  double sum = 0;
  double temp;
  int N = 5;
  char input[20];

  for (int i = 0; i < N; i++) {
    if (scanf("%lf", &temp)) {
      sum += temp;
      continue;
    }
    if (scanf("%s", &input) == 0) {
      if (strcmp(input, "STOP")){
      	break;
      } else {
        i--;
      }
    }
  }

  double avg = sum / N;
  printf("%lf", avg);
  
}
