FROM node:alpine

# Create app directory
WORKDIR /app

# Install system requirements
RUN apk update && apk add --no-cache python3 py3-pip build-base gcc wget git

# Install app dependencies
# A wildcard is used to ensure both package.json AND package-lock.json are copied
# where available (npm@5+)
COPY package*.json ./
RUN npm install --silent > /dev/null

# Copy app
COPY . .

# Run command 
ENTRYPOINT [ "node", "index.js" ]
CMD [ "" ]
