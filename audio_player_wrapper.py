import os
import fcntl
import subprocess
import threading
import getpass
from gmusicapi import Mobileclient

class AudioPlayerProtocolV1:
    def play_message_for(self, stream_url):
        return "play {0}\n".format(stream_url)
    
def set_non_blocking(fd):
    """
    Set the file description of the given file descriptor to non-blocking.
    """
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    flags = flags | os.O_NONBLOCK
    fcntl.fcntl(fd, fcntl.F_SETFL, flags)

class AudioPlayer:
    def __init__(self, audio_player_protocol = AudioPlayerProtocolV1()):
        self._audio_player_process = None
        self._is_listening_to_audio_player = False
        self._audio_player_listener_thread = None
        self._audio_player_protocol = audio_player_protocol
    
    def audio_player_process(self):
        return self._audio_player_process

    def audio_player_protocol(self):
        return self._audio_player_protocol

    def open(self):
        self._audio_player_process = subprocess.Popen("./a.out", \
                                                      stdin=subprocess.PIPE, \
                                                      stderr=subprocess.PIPE, \
                                                      stdout=subprocess.PIPE)

        self.listen_to_audio_player_process()

    def close(self):
        self._is_listening_to_audio_player = False
        self._audio_player_process.kill()

    def listen_to_audio_player_process(self):
        self._is_listening_to_audio_player = True
        self._audio_player_listener_thread = threading.Thread(target=self.listen)
        self._audio_player_listener_thread.start()
    
    def listen(self):
        while self._is_listening_to_audio_player:
            output = self._audio_player_process.stdout.readline()
            if output:
                print(output)

    def send_to_audio_player_process(self, message):
        self.audio_player_process().stdin.write(bytes(message, 'utf-8'))
        self.audio_player_process().stdin.flush()
        
    def play(self, stream_url):
        play_message = self.audio_player_protocol().play_message_for(stream_url)
        print(play_message)
        self.send_to_audio_player_process(play_message)

class MusicService:
    def __init__(self, mobile_client):
        self._mobile_client = mobile_client
    
    def mobile_client(self):
        return self._mobile_client

def get_authenitcated_client():
    email = input("Email: ")
    password = getpass.getpass("Password: ")

    client = Mobileclient()
    client.login(email, password, Mobileclient.FROM_MAC_ADDRESS)
    
    return client

def get_stream(client, search_term):
    song_store_id = client.search(search_term)['song_hits'][0]['track']['storeId']
    return client.get_stream_url(song_store_id)

if __name__ == "__main__":
    client = get_authenitcated_client()
    player = AudioPlayer()
    stream_url = get_stream(client, input("search for: "))
    player.open()
    player.play(stream_url)
    input("...")
    player.close()