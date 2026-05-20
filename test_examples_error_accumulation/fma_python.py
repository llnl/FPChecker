import math
import numpy as np

def fma_precision_error(x: float, y: float, z: float):
  """
  Computes the FMA of three values in single and double precision,
  and then calculates the error between the two results.

  Args:
    x: The first float value.
    y: The second float value.
    z: The third float value.
  """

  # Convert to single precision (32-bit float)
  #x_single = np.float32(x)
  #y_single = np.float32(y)
  #z_single = np.float32(z)

  x_single = x
  y_single = y
  z_single = z

  # Compute FMA in single precision
  fma_single_result = math.fma(x_single, y_single, z_single)

  # Extend to double precision (64-bit float)
  x_double = np.float64(x)
  y_double = np.float64(y)
  z_double = np.float64(z)

  # Compute FMA in double precision
  fma_double_result = math.fma(x_double, y_double, z_double)

  # Compute the error
  error = fma_double_result - np.float64(fma_single_result)

  # Print the results
  print(f"Single Precision FMA Result: {fma_single_result}")
  print(f"Double Precision FMA Result: {fma_double_result}")
  print(f"Error (Difference): {error}")


# Example usage of the function
if __name__ == '__main__':
  # Define three float values
  x=np.float32(2.9999999e-02)
  y=np.float32(2.9999999e-02)
  z=np.float32(0.0)

  # Calculate and display the FMA precision error
  fma_precision_error(x, y, z)
