import pvlib
import pandas as pd
import matplotlib.pyplot as plt

# Données spécifiques à votre installation
latitude = 43.49369  # Latitude de l'installation
longitude = 1.36243  # Longitude de l'installation
altitude = 160  # Altitude de l'installation en mètres
surface = 20  # Surface totale des panneaux en mètres carrés
puissance_crête = 4100  # Puissance crête totale en watts
orientation = 245  # Orientation des panneaux en degrés (0° = nord, 90° = est, etc.)
inclinaison = 20  # Inclinaison des panneaux en degrés

# Générer une plage horaire pour la simulation (par exemple, sur une journée)
date_range = pd.date_range(start="2023-08-29 06:00:00", end="2023-08-29 18:00:00", freq="15T")

# Obtenir les données de position solaire
solar_position = pvlib.solarposition.get_solarposition(date_range, latitude, longitude, altitude)

# Calculer la radiation solaire sur une surface inclinée
dni_extra = pvlib.irradiance.get_extra_radiation(date_range)
total_irrad = pvlib.irradiance.get_total_irradiance(
    surface_tilt=inclinaison,
    surface_azimuth=orientation,
    solar_zenith=solar_position['apparent_zenith'],
    solar_azimuth=solar_position['azimuth'],
    dni=dni_extra
)

# Calculer la puissance de sortie en utilisant le modèle PVWatts
effective_irradiance = total_irrad['poa_global']
pvwatts_dc = pvlib.pvsystem.pvwatts_dc(effective_irradiance, surface, puissance_crête)

i=0
for date in date_range:
    print(date, pvwatts_dc[i])
    i += 1


# Tracer la courbe de production
plt.figure(figsize=(10, 6))
plt.plot(date_range, pvwatts_dc)
plt.xlabel("Heure")
plt.ylabel("Puissance (W)")
plt.title("Courbe de production photovoltaïque")
plt.grid()
plt.show()