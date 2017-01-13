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

    def reset_message(self):
        return "reset \n"

    def color_message(self, level, red, green, blue):
        return "color {} {} {} {}\n".format(level, red, green, blue)

    def refresh_rate(self, period):
        return "refresh {}\n".format(period)

class AudioPlayer():
    def __init__(self, audio_player_protocol = AudioPlayerProtocolV1()):
        self._audio_player_process = None
        self._is_listening_to_audio_player = False
        self._audio_player_listener_thread = None
        self._audio_player_protocol = audio_player_protocol

        # event handlers
        self._on_song_finished_listener = None

    def with_on_song_finished_listener(self, listener):
        self._on_song_finished_listener = listener

    def dispatch_on_song_finished(self):
        if self._on_song_finished_listener:
            self._on_song_finished_listener()

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
            output = self._audio_player_process.stdout.readline().strip()
            if output:
                if (output == "finished")
                    self.dispatch_on_song_finished()

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
    
    def playlist_results_for(self, search_term, max_results=10):
        playlist_hits = self.mobile_client().search(search_term)['playlist_hits']
        playlist_results = [hit['playlist'] for hit in playlist_hits]
        results_to_return = min(max_results, len(playlist_results))

        return playlist_results[:results_to_return]

    def get_tracks_for(self, playlist_share_token):
        track_hits = self.mobile_client().get_shared_playlist_contents(playlist_share_token)
        tracks = [hit['track'] for hit in track_hits]

        return tracks

    def get_stream_for(self, track):
        return self.mobile_client().get_stream_url(track['storeId'])

class TrackList:
    def __init__(self):
        self.clear()
    
    def clear(self):
        self._tracks = []
        self._position = 0

    def tracks(self):
        return self._tracks

    def position(self):
        return self._position

    def set_position(self, new_position):
        self._position = new_position

    def increment_track(self):
        self._position += 1

    def add_track(self, track):
        self._tracks.append(track)
    
    def remove_track_at(self, position):
        self._tracks.pop(position)

    def get_current_track(self):
        return self._tracks[self.position()]

class Application:
    def __init__(self, audio_player, music_service):
        self.audio_player = audio_player
        self.music_service = music_service
        self.track_list = TrackList()
        self.active = True
        self.commands = { "1" : self.play_song,
                          "2" : self.play_playlist,
                          "3" : self.pause_song,
                          "4" : self.unpause_song,
                          "4" : self.stop_song,
                          "6" : self.set_volume,
                          "7" : self.exit }

    def run(self):
        self.audio_player.open()

        while self.active:
            print("What do you want to do")
            print("1. Search for a song")
            print("2. Search for a playlist")
            print("3. Pause current song")
            print("4. Unpause current song")
            print("5. Stop current song")
            print("6. Set volume")
            print("7. Exit")

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
    
    def play_playlist(self):
        search_term = input("Search for playlist: ")
        print()

        playlist_results = self.music_service.playlist_results_for(search_term)

        print("Select song to play:")
        for item in enumerate(playlist_results):
            print("{}. {}".format(item[0] + 1, item[1]['name']))
        print()

        playlist_index = int(input("Select playlist: ")) - 1
        print()

        selected_playlist_token = playlist_results[playlist_index]['shareToken']
        tracks = self.music_service.get_tracks_for(selected_playlist_token)

        self.track_list.clear()

        for track in tracks:
            self.track_list.add_track(track)
        
        stream = self.music_service.get_stream_for(self.track_list.get_current_track())
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

    client = Mobileclient(debug_logging = False)
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