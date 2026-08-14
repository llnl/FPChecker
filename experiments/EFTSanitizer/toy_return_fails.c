#include <stdio.h>

  float heat_flux(float conductivity, float temp_left, float temp_right, float dx) {
      return conductivity * (temp_right - temp_left) / dx;
  }

  float update_temperature(float temp, float flux, float dt, float density) {
      return temp + dt * flux / density;
  }

  int main(void) {
      float conductivity = 0.8f;
      float temp_left = 72.0f;
      float temp_right = 75.5f;
      float dx = 0.05f;

      float dt = 0.01f;
      float density = 1.2f;

      float flux = heat_flux(conductivity, temp_left, temp_right, dx);
      float next_temp = update_temperature(temp_left, flux, dt, density);

      printf("flux = %.8f\n", flux);
      printf("next temperature = %.8f\n", next_temp);

      return 0;
  }