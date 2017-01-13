import os
import fcntl
import subprocess
import threading
import getpass
import signal
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
            output = self._audio_player_process.stdout.readline().strip().decode('ascii')
            if output:
                if "finished" in output:
                    self.dispatch_on_song_finished()

    def send_to_audio_player_process(self, message):
        self.audio_player_process().stdin.write(bytes(message, 'ascii'))
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

    def set_color(self, level, red, green, blue):
        set_color_message = self.audio_player_protocol().color_message(level, red, green, blue)
        self.send_to_audio_player_process(set_color_message)

    def set_refresh_rate(self, period):
        set_refresh_message = self.audio_player_protocol().refresh_rate(period)
        self.send_to_audio_player_process(set_refresh_message)

    def reset(self):
        reset_message = self.audio_player_protocol().reset_message()
        self.send_to_audio_player_process(reset_message)

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

    def decrement_track(self):
        self._position -= 1

    def has_previous_track(self):
        return self._position != 0

    def has_next_track(self):
        return self._position + 1 < len(self._tracks)

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
                          "3" : self.increment_track_and_play,
                          "4" : self.decrement_track_and_play,
                          "5" : self.pause_song,
                          "6" : self.unpause_song,
                          "7" : self.stop_song,
                          "8" : self.set_volume,
                          "9" : self.set_refresh,
                          "10" : self.change_color,
                          "11" : self.reset_player }

        self.audio_player.with_on_song_finished_listener(self.on_song_finished)

    def run(self):
        self.audio_player.open()

        while self.active:
            print("What do you want to do")
            print("1. Search for a song")
            print("2. Search for a playlist")
            print("3. Next Song")
            print("4. Previous Song")
            print("5. Pause current song")
            print("6. Unpause current song")
            print("7. Stop current song")
            print("8. Set volume")
            print("9. Set refresh")
            print("10. Change color")
            print("11. Reset player")

            command = input("")
            print()
            if command in self.commands:
                self.commands[command]()
            else:
                print("{} is not an option.\n".format(command))

    def stop(self):
        self.active = False

    def print_current_song(self):
        current_track = self.track_list.get_current_track()
        print("Title: {}".format(current_track['title']))
        print("Album: {}".format(current_track['album']))
        print("Artist: {}".format(current_track['artist']))
        print()

    def get_selection(self, items, item_name = "item", item_to_string = lambda item : print(item)):
        print("Select a(n) {}: ".format(item_name))

        for item in enumerate(items):
            print("{}. {}".format(item[0] + 1, item_to_string(item[1])))

        none_of_the_above_index = len(items)
        print("{}. None of the above\n".format(none_of_the_above_index + 1))

        selected_index = get_int_input("Select {}: ".format(item_name)) - 1
        print()

        if selected_index != none_of_the_above_index:
            return selected_index
        return None

    def play_song(self):
        search_term = input("Search for song: ")
        print()

        song_hits = self.music_service.search_results_for(search_term)

        def song_hit_to_string(song_hit):
            track = song_hit['track']
            title = track['title'].encode('ascii', 'ignore').decode('ascii')
            album = track['album'].encode('ascii', 'ignore').decode('ascii')
            artist = track['artist'].encode('ascii', 'ignore').decode('ascii')
            return "{} from {} by {}".format(title, album, artist)

        selected_song_index = self.get_selection(song_hits, "song", song_hit_to_string)

        if selected_song_index is not None:
            selected_track = song_hits[selected_song_index]['track']

            self.set_tracks([selected_track])
            self.play_current_track()

    def play_playlist(self):
        search_term = input("Search for playlist: ")
        print()

        playlist_results = self.music_service.playlist_results_for(search_term)

        def playlist_hit_to_string(playlist_hit):
            title = playlist_hit['name'].encode('ascii', 'ignore').decode('ascii')
            return title

        selected_playlist_index = self.get_selection(playlist_results, "playlist", playlist_hit_to_string)

        if selected_playlist_index is not None:
            selected_playlist_token = playlist_results[selected_playlist_index]['shareToken']
            tracks = self.music_service.get_tracks_for(selected_playlist_token)

            self.set_tracks(tracks)
            self.play_current_track()

    def set_tracks(self, tracks):
        self.track_list.clear()

        for track in tracks:
            self.track_list.add_track(track)

    def decrement_track_and_play(self):
        if self.track_list.has_previous_track():
            self.track_list.decrement_track()
            self.play_current_track()

    def increment_track_and_play(self):
        if self.track_list.has_next_track():
            self.track_list.increment_track()
            self.play_current_track()

    def play_current_track(self):
        stream = self.music_service.get_stream_for(self.track_list.get_current_track())
        self.audio_player.play(stream)

        print("Now playing...")
        self.print_current_song()

    def pause_song(self):
        self.audio_player.pause()

    def unpause_song(self):
        self.audio_player.unpause()

    def stop_song(self):
        self.audio_player.stop()

    def change_color(self):
        level = get_int_input("Level: ")
        red = get_int_input("Red: ")
        green = get_int_input("Green: ")
        blue = get_int_input("Blue: ")
        print()

        self.audio_player.set_color(level, red, green, blue)


    def on_song_finished(self):
        if self.track_list.has_next_track():
            self.track_list.increment_track()
            self.play_current_track()

    def set_volume(self):
        volume_percentage = get_int_input("New volume: ")
        print()
        self.audio_player.set_volume(volume_percentage)

    def set_refresh(self):
        refresh_period = get_int_input("New refresh period: ")
        print()
        self.audio_player.set_refresh_rate(refresh_period)

    def reset_player(self):
        self.audio_player.reset()

def get_int_input(prompt):
    int_transform = lambda x : int(x)
    input_is_int = lambda x : x.isdigit()

    return get_input(prompt, int_transform, input_is_int)

def get_input(prompt = "", input_transform = lambda x : x, input_is_valid = lambda x : True):
    user_input = input(prompt)
    while not input_is_valid(user_input):
        print("Input is invalid, try again.")
        user_input = input(prompt)
    return input_transform(user_input)

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

    def sig_handler(signal, frame):
        audio_player.close()
        app.stop()
        print("Goodbye.")
        exit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    app.run()