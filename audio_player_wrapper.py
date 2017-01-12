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
        return "pause\n"
    
    def unpause_message(self):
        return "unpause\n"
    
    def stop_message(self):
        return "stop\n"
    
    def volume_with(self, percentage):
        return "volumne {0}\n".format(percentage)
    
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

    def search_results_for(self, search_term, max_results=10):
        #song_store_id = self.mobile_client().search(search_term)['song_hits'][0]['track']['storeId']
        #return self.mobile_client().get_stream_url(song_store_id)
        search_results = self.mobile_client().search(search_term)['song_hits']
        results_to_return = min(max_results, len(search_results))
        
        return search_results[:results_to_return]
    
    def get_stream_for(self, track):
        return self.mobile_client().get_stream_url(track['storeId']) 

def get_authenitcated_client():
    email = input("Email: ")
    password = getpass.getpass("Password: ")

    client = Mobileclient()
    client.login(email, password, Mobileclient.FROM_MAC_ADDRESS)
    
    if not client.is_authenticated():
        print("Failied to authenticate, try again.")
        return get_authenitcated_client()
    return client

class Application:
    def __init__(self, audio_player, music_service):
        self.audio_player = audio_player
        self.music_service = music_service
        self.active = True
        self.commands = { "1" : self.play_song,
                          "2" : self.exit }

    def run(self):
        self.audio_player.open()

        while self.active:
            print("What do you want to do")
            print("1. Search for a song")
            print("2. Exit")

            command = input("")
            if command in self.commands:
                self.commands[command]()
            else:
                print("{} is not an option.".format(command))
    
    def play_song(self):
        search_term = input("Search for: ")
        song_results = self.music_service.search_results_for(search_term)

        print("Select song to play:")
        for item in enumerate(song_results):
            print("{}. {} - {}".format(item[0] + 1, item[1]['track']['title'], item[1]['track']['album']))
        
        song_index = int(input("Select song: ")) - 1
        selected_track = song_results[song_index]['track']
        stream = self.music_service.get_stream_for(selected_track)

        self.audio_player.play(stream)
    
    def exit(self):
        self.audio_player.close()
        self.active = False

if __name__ == "__main__":
    client = get_authenitcated_client()
    
    audio_player = AudioPlayer()
    music_service = MusicService(client)

    app = Application(audio_player, music_service)
    app.run()