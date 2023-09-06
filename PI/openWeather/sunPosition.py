import pvlib
import pandas as pd

# Coordonnées géographiques (latitude, longitude)
latitude = 43.49369
longitude = 1.36243


# Date et heure


for hour in range(0, 24):
    if hour < 10:
        hour_str = "0" + str(hour)
    else:
        hour_str = str(hour)

    date = pd.Timestamp("2023-08-28 "+ hour_str + ":00:00")

    # Calculs
    solpos = pvlib.solarposition.get_solarposition(date, latitude, longitude)
    altitude = solpos['elevation'].values[0]
    azimuth = solpos['azimuth'].values[0]


    print(date, altitude, azimuth)
    #print("Azimuth du soleil :", azimuth)


