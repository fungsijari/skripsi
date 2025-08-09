import numpy as np
import matplotlib.pyplot as plt

#data
volume_ml = np.array([20,20,20, 40,40,40, 60,60,60, 80,80,80, 100,100,100])
vms_hum = np.array([34.6,34.8,35.0, 58.6,58.8,59.0, 73.6,73.8,74.0, 80.8,81.0,81.2, 89.6,89.8,90.0])
dig_hum = np.array([35.95,35.76,35.99, 60.41,59.88,59.88, 75.47,75.23,74.86, 81.86,81.86,82.07, 90.63,90.50,90.50])

#function
def trapmf(x, a, b, c, d):
    y = np.zeros_like(x, dtype=float)
    idx = (x > a) & (x < b)
    if b != a: y[idx] = (x[idx] - a) / (b - a)
    idx = (x >= b) & (x <= c)
    y[idx] = 1.0
    idx = (x > c) & (x < d)
    if d != c: y[idx] = (d - x[idx]) / (d - c)
    return y

def trimf(x, a, b, c):
    y = np.zeros_like(x, dtype=float)
    idx = (x > a) & (x < b)
    if b != a: y[idx] = (x[idx] - a) / (b - a)
    idx = (x > b) & (x < c)
    if c != b: y[idx] = (c - x[idx]) / (c - b)
    y[x == b] = 1.0
    return y

#parameter
params = {
    'Dry'      : (0.0, 7.0, 29.0, 35.0),   #trapesium
    'Moist'    : (30.0, 50.0, 70.0),        #segitiga
    'Wet'      : (60.0, 60.0, 80.0, 80.0),  #kotak
    'Very Wet' : (75.0, 90.0, 100.0, 100.0) #trapesium
}

#fuzzifikasi
mu_dry   = trapmf(vms_hum, *params['Dry'])
mu_moist = trimf (vms_hum, *params['Moist'])
mu_wet   = trapmf(vms_hum, *params['Wet'])
mu_vwet  = trapmf(vms_hum, *params['Very Wet'])

#tabel hasil
header = f"{'Vol(mL)':<8}{'VMS(%)':<8}{'Dry':<6}{'Moist':<6}{'Wet':<6}{'VWet':<6}{'Dig(%)':<8}{'Err(%)':<8}"
print(header); print("-"*len(header))
for vol, vms, d, m, w, vw, dig in zip(volume_ml, vms_hum, mu_dry, mu_moist, mu_wet, mu_vwet, dig_hum):
    err = abs(vms - dig) / dig * 100.0
    print(f"{vol:<8}{vms:<8.2f}{d:<6.2f}{m:<6.2f}{w:<6.2f}{vw:<6.2f}{dig:<8.2f}{err:<8.2f}")

#kurva
x = np.linspace(0, 100, 500)
y_dry   = trapmf(x, *params['Dry'])
y_moist = trimf (x, *params['Moist']) 
y_wet   = trapmf(x, *params['Wet'])
y_vwet  = trapmf(x, *params['Very Wet'])

fig, ax = plt.subplots(figsize=(9,5))
l1, = ax.plot(x, y_dry,   linewidth=2, label='Dry')
l2, = ax.plot(x, y_moist, linewidth=2, label='Moist')
l3, = ax.plot(x, y_wet,   linewidth=2, label='Wet')
l4, = ax.plot(x, y_vwet,  linewidth=2, label='Very Wet')

ax.fill_between(x, 0, y_dry,   where=(y_dry>0),   alpha=0.2, interpolate=True, color=l1.get_color())
ax.fill_between(x, 0, y_moist, where=(y_moist>0), alpha=0.2, interpolate=True, color=l2.get_color())
ax.fill_between(x, 0, y_wet,   where=(y_wet>0),   alpha=0.2, interpolate=True, color=l3.get_color())
ax.fill_between(x, 0, y_vwet,  where=(y_vwet>0),  alpha=0.2, interpolate=True, color=l4.get_color())

ax.scatter(vms_hum, mu_dry,   marker='o', zorder=5)
ax.scatter(vms_hum, mu_moist, marker='s', zorder=5)
ax.scatter(vms_hum, mu_wet,   marker='^', zorder=5)
ax.scatter(vms_hum, mu_vwet,  marker='D', zorder=5)

ax.set_xlim(0,100); ax.set_ylim(0,1.05)
ax.set_xlabel('Kelembaban VMS (%)'); ax.set_ylabel('Derajat Keanggotaan μ')
ax.set_title('Kelembaban'); ax.grid(True); ax.legend()
plt.tight_layout(); plt.show()
