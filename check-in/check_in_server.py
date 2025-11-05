import csv

from flask import Flask

app = Flask(__name__)


def clean_mac_address(mac_address: str) -> str:
    # Removes all separator and non-hex characters from a MAC address, returns the lowercase clean MAC address
    mac_lower: str = mac_address.lower()
    valid_mac_chars: set[str] = set("1234567890abcdef")
    mac_address_clean: str = "".join(
        [char for char in mac_lower if char in valid_mac_chars]
    )
    return mac_address_clean


def get_host_by_mac_address(target_mac_address: str) -> str:
    # Returns the host corresponding with `target_mac_address` in hosts.csv regardless of formatting
    target_clean_mac: str = clean_mac_address(target_mac_address)
    with open("hosts.csv", "r") as f:
        csv_reader = csv.DictReader(f)
        for row in csv_reader:
            mac: str = row["mac_address"]
            host: str = row["host"]
            clean_mac: str = clean_mac_address(mac)
            if target_clean_mac == clean_mac:
                return host

    return ""


@app.route("/client/get-server/<string:mac_address>")
def get_server(mac_address: str):
    return get_host_by_mac_address(mac_address)


if __name__ == "__main__":
    app.run(debug=True, port=5512)
