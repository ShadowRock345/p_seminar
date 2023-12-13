package com.apps4mondo.mqtt_broker

import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.widget.EditText
import androidx.activity.ComponentActivity
import androidx.appcompat.app.AlertDialog

import androidx.navigation.ui.AppBarConfiguration

import com.apps4mondo.mqtt_broker.databinding.ActivityMainBinding

import org.eclipse.paho.client.mqttv3.MqttCallbackExtended
import org.eclipse.paho.client.mqttv3.MqttException
import org.eclipse.paho.client.mqttv3.MqttMessage
import java.util.Timer

import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.delay

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import java.util.Calendar
import kotlin.concurrent.schedule


class MainActivity : ComponentActivity() {


    private lateinit var appBarConfiguration: AppBarConfiguration
    private lateinit var binding: ActivityMainBinding
    private lateinit var mqttClient: MqttClientHelper

    private var mqttHost: String = ""


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        showMqttHostInputDialog();





    }

    private fun showMqttHostInputDialog() {
        val builder = AlertDialog.Builder(this)
        builder.setTitle("MQTT-Host-Adresse eingeben")

        val input = EditText(this)
        builder.setView(input)

        builder.setPositiveButton("OK") { dialog, _ ->
            val newMqttHost = input.text.toString().trim()

            if (newMqttHost.isNotEmpty()) {
                mqttHost = newMqttHost

                initializeUI()
            } else {
                Snackbar.make(binding.textViewNumMsgs, "Adress is empty!", Snackbar.LENGTH_INDEFINITE)
                    .setAction("Action", null).show()
            }
            dialog.dismiss()
        }

        builder.setCancelable(false)
        builder.show()
    }

    private fun initializeUI() {

        mqttClient = MqttClientHelper(this, mqttHost);

        setMqttCallBack()

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        //setSupportActionBar(binding.toolbar)
        binding.textViewMsgPayload.movementMethod = ScrollingMovementMethod()
        binding.textViewMsgPayload.movementMethod = ScrollingMovementMethod()

        setMqttCallBack()

        // initialize 'num msgs received' field in the view
        binding.textViewNumMsgs.text = "0"

        // pub button
        binding.btnPub.setOnClickListener { view ->
            var snackbarMsg : String
            val topic = binding.editTextPubTopic.text.toString().trim()
            snackbarMsg = "Cannot publish to empty topic!"
            if (topic.isNotEmpty()) {
                snackbarMsg = try {
                    mqttClient.publish(topic, binding.editTextMsgPayload.text.toString())
                    "Published to topic '$topic'"
                } catch (ex: MqttException) {
                    "Error publishing to topic: $topic"
                }
            }
            Snackbar.make(view, snackbarMsg, 300)
                .setAction("Action", null).show()

        }

        binding.switch1.setOnCheckedChangeListener { view , isChecked ->
            val message = if (isChecked) "Switch1:ON" else "Switch1:OFF"
            Snackbar.make(binding.textViewNumMsgs, "Button changed: " + message, Snackbar.LENGTH_INDEFINITE)
                .setAction("Action", null).show()
            var snackbarMsg : String
            val topic = binding.editTextPubTopic.text.toString().trim()
            if (message.equals("Switch1:ON")) {
                snackbarMsg = "Cannot publish to empty topic!"
                if (topic.isNotEmpty()) {
                    snackbarMsg = try {
                        mqttClient.publish(topic, "Switch1:ON")
                        "Published to topic '$topic'"
                    } catch (ex: MqttException) {
                        "Error publishing to topic: $topic"
                    }
                }
                Snackbar.make(view, snackbarMsg, 300)
                    .setAction("Action", null).show()

            } else if (message.equals("Switch1:OFF")){
                snackbarMsg = "Cannot publish to empty topic!"
                if (topic.isNotEmpty()) {
                    snackbarMsg = try {
                        mqttClient.publish(topic, "Switch1:OFF")
                        "Published to topic '$topic'"
                    } catch (ex: MqttException) {
                        "Error publishing to topic: $topic"
                    }
                }
                Snackbar.make(view, snackbarMsg, 300)
                    .setAction("Action", null).show()
            }
        }

        // sub button
        binding.btnSub.setOnClickListener { view ->
            var snackbarMsg : String
            val topic = binding.editTextSubTopic.text.toString().trim()
            snackbarMsg = "Cannot subscribe to empty topic!"
            if (topic.isNotEmpty()) {
                snackbarMsg = try {
                    mqttClient.subscribe(topic)
                    "Subscribed to topic '$topic'"
                } catch (ex: MqttException) {
                    "Error subscribing to topic: $topic"
                }
            }
            Snackbar.make(view, snackbarMsg, Snackbar.LENGTH_SHORT)
                .setAction("Action", null).show()
        }

        Timer("CheckMqttConnection", false).schedule(3000) {
            if (!mqttClient.isConnected()) {
                Snackbar.make(binding.textViewNumMsgs, "Failed to connect to: '$mqttHost' within 3 seconds", Snackbar.LENGTH_INDEFINITE)
                    .setAction("Action", null).show()
            }
        }
    }

    private fun setMqttCallBack() {
        mqttClient.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(b: Boolean, s: String) {
                val snackbarMsg = "Connected to host:\n'$MQTT_HOST'."
                Log.w("Debug", snackbarMsg)
                Snackbar.make(findViewById(android.R.id.content), snackbarMsg, Snackbar.LENGTH_LONG)
                    .setAction("Action", null).show()
            }
            override fun connectionLost(throwable: Throwable) {
                val snackbarMsg = "Connection to host lost:\n'$MQTT_HOST'"
                Log.w("Debug", snackbarMsg)
                Snackbar.make(findViewById(android.R.id.content), snackbarMsg, Snackbar.LENGTH_LONG)
                    .setAction("Action", null).show()
            }
            @Throws(Exception::class)
            override fun messageArrived(topic: String, mqttMessage: MqttMessage) {
                Log.w("Debug", "Message received from host '$MQTT_HOST': $mqttMessage")
                binding.textViewNumMsgs.text = ("${binding.textViewNumMsgs.text.toString().toInt() + 1}")
                val str: String = "------------"+ Calendar.getInstance().time +"-------------\n$mqttMessage\n${binding.textViewMsgPayload.text}"
                binding.textViewMsgPayload.text = str
            }

            override fun deliveryComplete(iMqttDeliveryToken: IMqttDeliveryToken) {
                Log.w("Debug", "Message published to host '$MQTT_HOST'")
            }
        })
    }

}

