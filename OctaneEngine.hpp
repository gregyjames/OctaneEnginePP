#ifndef OCTANEENGINE_H
#define OCTANEENGINE_H
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <curl/curl.h>
#include <fstream>
#include <math.h>
#include "tqdm.h"
using namespace std;

class dl_part{
    private:
        //Info for each part
        string url_obj;
        double start_file;
        double end_file;
        int file_id;
        static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        };
        static int progress_func(void* ptr, double TotalToDownload, double NowDownloaded,double TotalToUpload, double NowUploaded)
        {
            tqdm bar;
            bar.reset();
            bar.set_theme_line();
            bar.progress(NowDownloaded, TotalToDownload);

            // and back to line begin - do not forget the fflush to avoid output buffering problems!
            fflush(stdout);
            // if you don't return 0, the transfer will be aborted - see the documentation
            return 0;
        }

    public:
        dl_part(){
            start_file = 0;
            end_file = 0;
            file_id = 0;
            url_obj = "";
        }
        dl_part(int id, string url, long start_part, long end_part){
            start_file = start_part;
            end_file = end_part;
            url_obj = url;
            file_id = id;
        }
        int get_fileid(){
            return file_id;
        }
        void download(){
            //The range arg to download
            string range = to_string(start_file) + "-" + to_string(end_file);
            //Curl and file object
            CURL *curl;
            FILE *fp;
            //Error buffer
            char errbuf[CURL_ERROR_SIZE];
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {
                //Open the file for writing
                fp = fopen(to_string(file_id).c_str(),"wb");
                //Set the URL of file to download
                curl_easy_setopt(curl, CURLOPT_URL, url_obj.c_str());
                //Provide a buffer to store errors in
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
                //Set the error buffer as empty before performing a request
                errbuf[0] = 0;
                //Set content range
                curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
                //Use custom write function (NULL MAY CRASH ON WINDOWS)
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                //Enables the progress bar
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                //Sets the progress function
                curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
                //Writes data to file
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                //Follow all redirects
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                res = curl_easy_perform(curl);
                //Print error if there is an error
                if(res != CURLE_OK) {
                    size_t len = strlen(errbuf);
                    fprintf(stderr, "\nlibcurl: (%d) ", res);
                    if(len)
                        fprintf(stderr, "%s%s", errbuf,((errbuf[len - 1] != '\n') ? "\n" : ""));
                    else
                        fprintf(stderr, "%s\n", curl_easy_strerror(res));
                }
                //Cleanup and close the file
                curl_easy_cleanup(curl);
                fclose(fp);
            }
        }
        void get_range(){
            printf ("The range of the part is from %f - ", start_file);
            printf ("%f\n", end_file);
        }
};
class OctaneEngine{
    private:
        double file_size;
        double part_size;
        string url_obj;
        string out_file;
        static size_t throw_away(void *ptr, size_t size, size_t nmemb, void *data)
        {
            //Only return the size of the data we would have saved
            (void)ptr;
            (void)data;
            return (size_t)(size * nmemb);
        }
        void get_file_size(){
            CURL *curl;
            char errbuf[CURL_ERROR_SIZE];
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {
                //The download url itself
                curl_easy_setopt(curl, CURLOPT_URL, url_obj.c_str());
                //Provide a buffer to store errors in
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
                //Follow all redirects
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                //Don't save the headers
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, throw_away);
                //No header in request
                curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
                //No body in request
                curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
                res = curl_easy_perform(curl);
                //If there was an error, print it
                if(res != CURLE_OK) {
                    size_t len = strlen(errbuf);
                    fprintf(stderr, "\nlibcurl: (%d) ", res);
                    if(len)
                        fprintf(stderr, "%s%s", errbuf,((errbuf[len - 1] != '\n') ? "\n" : ""));
                    else
                        fprintf(stderr, "%s\n", curl_easy_strerror(res));
                }
                else{
                    //Get the content length if successful
                    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,&file_size);
                }
                curl_easy_cleanup(curl);
            }
        }
        void join_parts(string outfile){
            std::ofstream of_c(outfile, std::ios_base::binary);
            for(int i = 0; i < (*(&parts + 1) - parts); i++){
                std::ifstream if_a(to_string(i).c_str(), std::ios_base::binary);
                of_c << if_a.rdbuf();
                remove(to_string(i).c_str());
            }
            of_c.close();
        }
        int NUMPARTS;
    public:
        dl_part parts[8];
        OctaneEngine(string url, string out){
            url_obj = url;
            out_file = out;
            //Get the file size of the remote file
            get_file_size();
            //Number of parts is equal to the size of the parts array
            NUMPARTS = (*(&parts + 1) - parts);
            part_size = file_size / NUMPARTS;
            int counter = 0;
            //Calculate the start and end for all part size objects
            for(double i = 0.0; i < file_size; i = i + part_size){
                parts[counter] = dl_part(counter, url_obj, floor(i), floor(i + part_size));
                counter = counter + 1;
            }

        }
        void get_url(){
            cout << url_obj <<endl;
        }
        void get_filesize(){
            printf ("The file size is:  %f\n", file_size);
        }
        void get_partsize(){
            printf ("The part size is:  %f\n", part_size);
        }
        void download_file(){
            //Starts the io service
            boost::asio::io_service io_service;
            boost::asio::io_service::work work(io_service);

            //Adds Handlers to IO service
            for(dl_part i: parts){
                io_service.post(boost::bind(&dl_part::download, i));
                i.get_range();
            }
            //Creates the worker threads
            boost::thread_group worker_threads;
            for(int x = 0; x < floor(NUMPARTS / 2); ++x)
            {
                worker_threads.create_thread(
                    boost::bind(&boost::asio::io_service::poll, &io_service)
                );
            }
            //Wait for the threads to finish
            worker_threads.join_all();
            //Join and clean all the individual parts
            join_parts(out_file);
        }
};
#endif
