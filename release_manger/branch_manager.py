#!/usr/bin/python3.6

import argparse
import os, glob



feature_branch_name_prefix = 'feature/'
release_branch_name_prefix = 'release/'
master_branch_name = 'master'
develop_branch_name = 'develop'


def merge_two_branches(curr,ref):
    print('Merging two branch')


def _validate_release_name(release_name):
    """
    Validate if the release name is valide. 
    Function does the following validation checks: 
        - version format is as vx.y.z, whereby v defines version, x,y,z is the version number, e.g., 0.0.9.
        - release version number is higher than the previous deployed release. (Not supported)

    Parameters
    ----------
    release_name: string 
        Name of the release List containing the data sample

    Returns
    -------
        : bool
        True if the name is valid, otherwise False
    """   
    if not (release_name):
        print('Release name not valid')
        return False

    return True



def _merge_two_branches(target_branch, reference_branch):
    """
    Merge two branches using git

    Parameters
    ----------
    target_branch: string 
        Name of the branch, namely the current branch, where the reference branch should be merged into

    target_branch: string 
        Name of the branch, namely the current branch, where the reference branch should be merged into
    
    reference_branch: string 
        Name of the branch, namely a next branch that is not the current branch, from which git should use to merge to the target branch


    Returns
    -------
        : bool
        Returns True if the merge was successfully started; Otherwise, False
    """   


    #ensure there are no modified files 
    # stream = os.popen('git stash')

    #Ensure the reference branch is up to date
    stream = os.popen('git checkout' +' ' + reference_branch)
    stream = os.popen('git pull origin')

    #Make sure we are on the target branch and it is up to date
    stream = os.popen('git checkout' +' ' + target_branch )
    stream = os.popen('git pull origin')


    #Merge the reference to the target branch
    stream = os.popen('git merge' + ' ' + reference_branch)



def start_release(release_name):
    """
    Start a release branch. Should be called in the local environment space.
    Function will create branch named release/release_name, whereby release_name 
    is the provided name for the release in string format, e.g, v9.0.0.

    Modified git files will automatically be stash. 

    Parameters
    ----------
    release_name: string 
        Name of the release List containing the data sample

    Returns
    -------
        : bool
        Returns True if release was successfully started; Otherwise, False
    """    

    #Confirm that the release name is good and contains the correct format v.x.y.z
    if (_validate_release_name(release_name)):
        print('Starting new release: '+ release_name)
        target_branch_name = release_branch_name_prefix + release_name
        
        print('branch name: ' + target_branch_name)

        # #ensure there are no modified files 
        # stream = os.popen('git stash')

        #Checkout lates develop branch
        stream = os.popen('git checkout' + ' ' + develop_branch_name)

        #TODO
        #if the repo exists on a server pull 
        stream = os.popen('git pull origin')           
        
        #Create release branch from the develop branch and push to the git server using GitFlow
        # stream = os.popen('git flow release start' +' ' + release_name) 
        # stream = os.popen('git flow rlease publish' +' ' + release_name)  

        #Create release branch from the develop branch and push to the git server
        stream = os.popen('git checkout -b' +' ' + target_branch_name) 
        stream = os.popen('git push -u origin' +' ' + target_branch_name)  

        return True 
    else:
        print('No release branch provided')
        return False


def finalize_release(release_name):
    """
    Finalize a release brach. Should be called in the local environment space.
    Upon approval, this function will proceed to finalizing the release branch by taking the last commited commit on the release branch
    and merging it to the master and develop branch with the release branch. Then the release branch will be deleted. It is therefore, expected 
    that the commits to the release branch with the desired messags be done before calling this function.

    The approval, which is requird before finalizing the release branch, depends on the software unit tests and approval of a user.
 
    Modified git files will automatically be stash. 

    Note: 
        Support to wait for feedback of Software unit tests and user approval are yet to be done

    Parameters
    ----------
    release_name: string 
        Name of the release List containing the data sample

    Returns
    -------
    n.a

    """     
    try:
        if(release_name):
            print('Finalizing release:'+release_name)
            target_branch_name = release_branch_name_prefix + release_name
            
            #ensure there are no modified files 
            stream = os.popen('git stash')

            #Ensure the develop branch is up todate 
            stream = os.popen('git checkout' + ' ' + develop_branch_name)
            stream = os.popen('git pull origin') 

            #Make sure we are on the latest release branch
            stream = os.popen('git checkout' + ' ' + target_branch_name)        

            #Wait for approval
           
            #Merge release branch to master and develop branch using GitFlow 
            # stream = os.popen('git flow release finish' + ' ' + release_name )
            
            #Merge release branch to develop branch and update server
            _merge_two_branches(target_branch_name, develop_branch_name)
            stream = os.popen('git checkout' + ' ' + develop_branch_name)
            stream = os.popen('git push origin')

            #Merge release branch to mastert branch and update server
            _merge_two_branches(target_branch_name, master_branch_name)
            stream = os.popen('git checkout' + ' ' + master_branch_name)
            stream = os.popen('git push origin')

            #Delete release branch locally and remotely
            stream = os.popen('git branch -d' + ' ' + target_branch_name )
            stream = os.popen('git push origin --delete' + ' ' + target_branch_name )


            #Push tags to Git server
            stream = os.popen('git push origin --tags')
        else:
            print('Release name not provided')

    except:
        print('Could not handle git commands to finalzie release')            


def _init_dir (git_dir):
    """
    Initialize a directory with git and gitflow. The default git flow values are used. 

    Parameters
    ----------
    git_dir: string 
        Path to a directory where git and git flow ought to be initalized and used.

    Returns
    -------
    n.a
    """     
    try:
        if (git_dir):
            #see if directory exists
            if os.path.exists(git_dir):
                print('directory found')

                os.chdir(git_dir)
                cwd = os.getcwd() 
                print(cwd)

                
                #initialize folder with git flow default settings
                #seems not to work from python !!
                # stream = os.popen('git flow init -f -d')



            else:
                print('Git directory does not exists')
        else:
            print('Git directory not provided')
    except OSError:
        print('Exception: File not found, no permission, or not a directory')

    except:
        print('Exception')


def main():
   # _init_dir('/home/javier/repo/deploy_test/')
   start_release('v1.0.0')


if __name__ == '__main__':
    main()