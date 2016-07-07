try:
    import urllib.request as urllib2
except ImportError:
    import urllib2
import shutil

try:
    print("Trying to fetch database file from the network...")
    response = urllib2.urlopen('http://zezulkanet.8u.cz/items.csv')
    with open("items.csv", "wb") as f:
        shutil.copyfileobj(response, f)
    print("Database file successfully downloaded!")
except:
    print("Database file download NOT successful, switching to local file...")
