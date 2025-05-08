#!/usr/bin/env python3
import os
import html
from urllib.parse import parse_qs
import email.utils
import sys

orders_file = "var/temp/coffee_orders.txt"


def read_orders():
    if not os.path.exists(orders_file):
        return []
    with open(orders_file, "r") as f:
        return f.readlines()

def main(): 

    html_body = f"""<!DOCTYPE html>
<html>
    <head>
        <title>All</title>
        <link rel="icon" type="image/png" href="/favicon.png" />
        <link rel="stylesheet" type="text/css" href="/style/coffee.css" />
    </head>
    <body>
        <h1>All Coffee Orders</h1>
        <ul>
"""

    # Read orders and format them as list items
    for order in read_orders():
        html_body += f"            <li>{order.strip()}</li>\n"

    # Close the unordered list and add a back link
    html_body += """        </ul>
    </body>
</html>
"""

	# Generate headers
    date_str = email.utils.formatdate(usegmt=True)
    content_length = len(html_body.encode("utf-8"))
    # Send the response headers and body
    print(f"HTTP/1.1 200 OK\r\n"
          f"Content-Type: text/html\r\n"
          f"Content-Length: {content_length}\r\n"
          f"\r\n"
          f"{html_body}")

if __name__ == "__main__":
    main()