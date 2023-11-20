var mqtt = require('mqtt');
var express = require('express');
const axios = require('axios');
const cors = require('cors');


const MqttServer = "172.20.10.3";
const MqttPort = "1883";
const MqttUser = "";
const MqttPass = "";

const app = express();
const port = 3000;

app.use(express.json());
app.use(cors());

var client = mqtt.connect({
    host: MqttServer,
    port: MqttPort,
    username: MqttUser,
    password: MqttPass
});

client.on('connect', function () {
    console.log("MQTT CONNECT");
    client.subscribe('dht11', function (err) {
        if (err) {
            console.log(err);
        }
    });
});

client.on('message', function (topic, message) {
    console.log('Received message:', message.toString());
    const sensorData = JSON.parse(message.toString());

    axios.post("http://172.20.10.3:4000/data", sensorData)
        .then(response => {
            console.log('Data Sent And To Server:', response.data);
        })
        .catch(error => {
            console.error('Error in Data Sending:', error.message);
        });
});

app.get('/click/:status', (req, res) => {
    const data = req.params.status

    client.publish("LED", data.toString());
    res.json({ message: 'Request Processed.' });
})

app.listen(port, () => {
    console.log(`Server listening on  https://localhost:${port}`);
});