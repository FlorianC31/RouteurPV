import requests
import json
from datetime import datetime, timedelta

# URL de l'API avec les coordonnées GPS et votre clé API
URL = "https://api.openweathermap.org/data/3.0/onecall"
API_KEY = "be7553c1fd40082e70e1d461f4b19541"
LAT = 43.49369
LONG = 1.36243

def forecast():
    url = URL + "?lat=" + str(LAT) + "&lon=" + str(LONG) + "&appid=" + API_KEY

    # Effectuer la requête HTTP GET
    response = requests.get(url)

    # Vérifier si la requête a réussi (code de statut 200)
    if response.status_code == 200:
        # Charger les données JSON à partir de la réponse
        data = response.json()
        for hour in data['hourly']:

            # Convertir le timestamp en objet datetime
            utc_datetime = datetime.fromtimestamp(hour['dt'])
            print(utc_datetime, hour['clouds'], hour['weather'][0]['main'], hour['weather'][0]['description'])

    else:
        print("La requête n'a pas abouti.")

def history(timestamp):
    dt = str(int(timestamp.timestamp()))
    url = URL + "/timemachine" + "?lat=" + str(LAT) + "&lon=" + str(LONG) + "&dt=" + dt + "&appid=" + API_KEY
    # Effectuer la requête HTTP GET
    response = requests.get(url)

    # Vérifier si la requête a réussi (code de statut 200)
    if response.status_code == 200:
        # Charger les données JSON à partir de la réponse
        data = response.json()
        return data['data']

    else:
        print("La requête n'a pas abouti.")

if __name__ == "__main__":
    
    timestamp = datetime(2023, 8, 28, 7, 1, 0)
    for m in range(0, 168):
        timestamp += timedelta(minutes=5)

        #print(timestamp, timestamp.timestamp())
        print(timestamp, history(timestamp))
