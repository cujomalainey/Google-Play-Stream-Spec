import os
import subprocess
from gmusicapi import Mobileclient

class AudioPlayerProtocolV1:
    def play_message_for(self, stream_url):
        return "play {0}\n".format(stream_url)
    
class AudioPlayer:
    def __init__(self, audio_player_protocol = AudioPlayerProtocolV1()):
        self._audio_player_process = None
        self._audio_player_protocol = audio_player_protocol
    
    def audio_player_process(self):
        return self._audio_player_process

    def audio_player_protocol(self):
        return self._audio_player_protocol

    def open(self):
        self._audio_player_process = subprocess.Popen("./audio_player", \
                                                      shell=True, \
                                                      stdin=subprocess.PIPE, \
                                                      stderr = subprocess.PIPE, \
                                                      stdout=subprocess.PIPE)
    def close(self):
        self._audio_player_process.kill()

    def send_to_audio_player_process(self, message):
        self.audio_player_process().communicate(input=bytes(message, 'UTF-8'))
        
    def play(self, stream_url):
        play_message = self.audio_player_protocol().play_message_for(stream_url)
        self.send_to_audio_player_process(play_message)

class MusicService:
    def __init__(self, mobile_client):
        self._mobile_client = mobile_client
    
    def mobile_client(self):
        return self._mobile_client

client = Mobileclient()
client.login(email, pwd, Mobileclient.FROM_MAC_ADDRESS)
print(client.search("okay"))