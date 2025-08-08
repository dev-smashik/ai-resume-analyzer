import React from 'react'
import Navbar from "~/components/Navbar";
import { useState } from 'react';
import type { Route } from "./+types/upload";
import FileUploader from "~/components/FileUploader";
import { usePuterStore } from "~/lib/putter";
import { useNavigate } from "react-router";
import {convertPdfToImage} from "~/lib/pdf2img";
import { generateUUID } from '~/lib/utils';



const upload = () => {
    const {auth, fs, isLoading, ai, kv} = usePuterStore();
    const navigate = useNavigate();
    const [isProcessing, setIsProcessing] = useState(false);
    const [statusText, setStatusText] = useState('');
    const [file, setFile] = useState<File | null>(null);
    const handleFileSelect = (file: File | null) => {
        setFile(file);
    }

    const handleAnalyze = async ({companyName, jobTitle, jobDescription, file} : {companyName: string, jobTitle: string, jobDescription: string, file: File}) => {
        setIsProcessing(true);
        setStatusText('Uploading the file ...');
        const uploadedFile = await fs.upload([file]);
        if(!uploadedFile) return setStatusText('File upload failed. Please try again.');

        setStatusText("Converting to image...");
        const imageFile = await convertPdfToImage(file);
        if(!imageFile.file) return setStatusText('Error: Failed to convert PDF to image.');


        setStatusText("Uploading the Image...");
        const uploadedImage = await fs.upload([imageFile.file]);
        if(!uploadedImage) return setStatusText('Image upload failed. Please try again.');

        setStatusText("Preparing Data...");

        const uuid = generateUUID();
    }
    const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
        e.preventDefault();
        const form = e.currentTarget.closest('form');
        if(!form) return;
        const formData = new FormData(form);

        const companyName = formData.get('company-name') as string;
        const jobTitle = formData.get('job-title') as string;
        const jobDescription = formData.get('job-description') as string;

        // console.log({companyName, jobTitle, jobDescription, file});
        if(!file) return;

        handleAnalyze({companyName, jobTitle, jobDescription, file});
                                                                                                                    
    }
  return (
    <main className="bg-[url('/images/bg-main.svg')] bg-cover">
      <Navbar />
      <section className='main-section'>
        <div className='paage-heading text-center py-16'>
            <h1>Smart Feedback For Your Dream Job</h1>
            {isProcessing ? (
                <>
                <h2>{statusText}</h2>
                <img src="public/images/resume-scan.gif" className='w-full' />
                </>
                
            ) : (
                <h2>Drop your resume for an ATS Score and Improvement tips</h2>
            )}

            {!isProcessing && (
                <form id='upload-form' onSubmit={handleSubmit} className='flex flex-col gap-4 mt-8'>
                    <div className='form-div'>
                        <label htmlFor="comapany-name">Company Name</label>
                        <input type="text" name='company-name' placeholder='Company Name' id='company-name' />
                    </div>
                    <div className='form-div'>
                        <label htmlFor="job-title">Job Title</label>
                        <input type="text" name='job-title' placeholder='Job Title' id='job-title' />
                    </div>
                    <div className='form-div'>
                        <label htmlFor="job-description">Job Description</label>
                        <textarea rows={5} name='job-description' placeholder='Job Description' />
                    </div>

                    <div className='form-div'>
                        <label htmlFor="uploader">Upload Resume</label>
                        <FileUploader onFileSelect={handleFileSelect}/>
                    </div>

                    <button className='primary-button' type='submit'> Analyze Resume </button>
                    </form> )}
        </div>
      </section>
      </main>
  )
}

export default upload