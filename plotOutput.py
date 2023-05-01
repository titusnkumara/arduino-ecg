import matplotlib.pyplot as plt

# Read the peak data from the output file
with open('output.txt', 'r') as f:
    peak_data = [line.strip().split(',') for line in f]
    peak_locations = [int(x[0]) for x in peak_data]
    y_values = [float(x[1]) / 1000 for x in peak_data]
    heart_rate = [float(x[2]) / 1000 for x in peak_data]

# Plot the original ECG data with peak locations and y-values marked
with open('data/100.txt', 'r') as f:
    ecg_data = [float(line.strip()) for line in f]
plt.subplot(2, 1, 1)
plt.plot(ecg_data)
plt.plot(peak_locations, [ecg_data[peak] for peak in peak_locations], 'o', label='Peaks')
plt.legend()
plt.title('Original ECG Data with Peak Locations')

# Plot the y-values of the peak locations
plt.subplot(2, 1, 2)
plt.plot(y_values, 'o')
plt.title('Y-Values of Peak Locations')

# Show the plots
plt.show()
