import os
import fcntl
import subprocess
import threading
import getpass
from gmusicapi import Mobileclient

class AudioPlayerProtocolV1:
    def play_message_for(self, stream_url):
        return "play {0}\n".format(stream_url)

    def pause_message(self):
        return "pause \n"

    def unpause_message(self):
        return "unpause \n"

    def stop_message(self):
        return "stop \n"

    def volume_with(self, percentage):
        return "volume {0}\n".format(percentage)

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
        self.send_to_audio_player_process(play_message)
    
    def pause(self):
        pause_message = self.audio_player_protocol().pause_message()
        self.send_to_audio_player_process(pause_message)

    def unpause(self):
        unpause_message = self.audio_player_protocol().unpause_message()
        self.send_to_audio_player_process(unpause_message)
    
    def stop(self):
        stop_message = self.audio_player_protocol().stop_message()
        self.send_to_audio_player_process(stop_message)
    
    def set_volume(self, volume_percentage):
        volume_message = self.audio_player_protocol().volume_with(volume_percentage)
        self.send_to_audio_player_process(volume_message)

class MusicService:
    def __init__(self, mobile_client):
        self._mobile_client = mobile_client
    
    def mobile_client(self):
        return self._mobile_client

    def search_results_for(self, search_term, max_results=10):
        search_results = self.mobile_client().search(search_term)['song_hits']
        results_to_return = min(max_results, len(search_results))
        
        return search_results[:results_to_return]
    
    def get_stream_for(self, track):
        return self.mobile_client().get_stream_url(track['storeId']) 

class Application:
    def __init__(self, audio_player, music_service):
        self.audio_player = audio_player
        self.music_service = music_service
        self.active = True
        self.commands = { "1" : self.play_song,
                          "2" : self.pause_song,
                          "3" : self.unpause_song,
                          "4" : self.stop_song,
                          "5" : self.set_volume,
                          "6" : self.exit }

    def run(self):
        self.audio_player.open()

        while self.active:
            print("What do you want to do")
            print("1. Search for a song")
            print("2. Pause current song")
            print("3. Unpause current song")
            print("4. Stop current song")
            print("5. Set volume")
            print("6. Exit")

            command = input("")
            print()
            if command in self.commands:
                self.commands[command]()
            else:
                print("{} is not an option.".format(command))
    
    def play_song(self):
        search_term = input("Search for: ")
        print()

        song_results = self.music_service.search_results_for(search_term)

        print("Select song to play:")
        for item in enumerate(song_results):
            print("{}. {} from {} by {}".format(item[0] + 1, item[1]['track']['title'], item[1]['track']['album'], item[1]['track']['artist']))
        print()
        
        song_index = int(input("Select song: ")) - 1
        print()
        selected_track = song_results[song_index]['track']
        stream = self.music_service.get_stream_for(selected_track)

        self.audio_player.play(stream)
    
    def pause_song(self):
        self.audio_player.pause()

    def unpause_song(self):
        self.audio_player.unpause()

    def stop_song(self):
        self.audio_player.stop()

    def set_volume(self):
        volume_percentage = input("New volume: ")
        print()
        self.audio_player.set_volume(volume_percentage)

    def exit(self):
        self.audio_player.close()
        self.active = False

def get_authenitcated_client():
    email = input("Email: ")
    password = getpass.getpass("Password: ")

    client = Mobileclient()
    client.login(email, password, Mobileclient.FROM_MAC_ADDRESS)
    
    if not client.is_authenticated():
        print("Failied to authenticate, try again.")
        return get_authenitcated_client()
    return client

if __name__ == "__main__":
    print("Sign into your Google Music account:\n")
    client = get_authenitcated_client()
    
    audio_player = AudioPlayer()
    music_service = MusicService(client)

    app = Application(audio_player, music_service)
    app.run()