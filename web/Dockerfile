FROM python:3.10.4

# Set working space
WORKDIR /usr/src/app

# Copy requirements
COPY requirements.txt .

# Install requirements
RUN pip install --no-cache-dir -r requirements.txt

# Copy app
COPY ./src .

# Start the bot
CMD python -u master_node.py
