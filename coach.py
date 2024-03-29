import logging
import os
import sys
from collections import deque
from pickle import Pickler, Unpickler
from random import shuffle
import json
from networks.NNet import args as nnargs
import numpy as np
from tqdm import tqdm
import matplotlib.pyplot as plt
import subprocess
import platform

log = logging.getLogger(__name__)


class Coach():
    """
    This class executes the self-play + learning. It uses the functions defined
    in Game and NeuralNet. args are specified in main.py.
    """

    def __init__(self, nnet, args):
        self.nnet = nnet
        self.pnet = self.nnet.__class__()  # the competitor network
        self.args = args
        self.trainExamplesHistory = []  # history of examples from args.numItersForTrainExamplesHistory latest iterations
        self.skipFirstSelfPlay = False  # can be overriden in loadTrainExamples()



    def learn(self):
        """
        Performs numIters iterations with numEps episodes of self-play in each
        iteration. After every iteration, it retrains neural network with
        examples in trainExamples (which has a maximum length of maxlenofQueue).
        It then pits the new neural network the old one and accepts it
        only if it wins >= updateThreshold fraction of games.
        """


        # Write init of serialized network
        # TODO: Check if one exists or pick up where we left off
        if(not self.args.load_model):
            self.nnet.save_checkpoint(folder=self.args.model_folder, filename= '0.pth.tar')
            self.nnet.save_checkpoint(folder=self.args.model_folder, filename='best.pth.tar')
        else:
            self.nnet.load_checkpoint(folder=self.args.model_folder, filename='best.pth.tar')

        
        bestModel = 'best.pth.tar'

        for i in range(0, self.args.numIters + 1):
            # bookkeeping
            log.info(f'Starting Iter #{i} ...')
            self.iteration = i 
            self.saveModelInfo()
            # Always use the best model for generating examples
            log.info('Calling selfplay subprocess')
            
            # Determine the extension based on the operating system
            extension = '.exe' if platform.system() == 'Windows' else ''

            # Construct the path to the executable
            executable_path = os.path.join('othello', 'build', 'Release', 'othello' + extension)
 
            subprocess.run([executable_path,
                            "selfplay",
                            str(self.args.selfplayGames),
                            str(self.args.selfplayMCTSSims),
                            self.args.model_folder + bestModel + '.pt',
                            self.args.model_folder + '/examples.json'])
        

            log.info('Loading training examples')
            self.loadTrainExamples()

            # Ensure we have < maxlenOfQueue examples, delete the oldest ones for space
            print('len hist', len(self.trainExamplesHistory))
            if(len(self.trainExamplesHistory) >= self.args.maxlenOfQueue):
                difference = len(self.trainExamplesHistory) - self.args.maxlenOfQueue
                del self.trainExamplesHistory[0:difference]
                print('len hist', len(self.trainExamplesHistory))


            # Copy examples then shuffle them
            trainExamples = []
            for e in self.trainExamplesHistory:
                trainExamples.append(e)
            shuffle(trainExamples)

            self.nnet.train(trainExamples)
            
            # currModel =  str(i) + '.pth.tar'
            # self.nnet.save_checkpoint(folder=self.args.model_folder, filename=currModel)
            self.nnet.save_checkpoint(folder=self.args.model_folder, filename='best.pth.tar')

            

    def getCheckpointFile(self, iteration):
        return 'checkpoint_' + str(iteration) + '.pth.tar'


    def loadTrainExamples(self):
        examplesFile = self.args.model_folder + "/examples.json"
        if not os.path.isfile(examplesFile):
            log.warning(f'File "{examplesFile}" with trainExamples not found!')
            r = input("Continue? [y|n]")
            if r != "y":
                sys.exit()
        else:
            log.info("File with trainExamples found. Loading it...")
            with open(examplesFile, "rb") as f:
                data = json.load(f)
                examples = data['examples']
                for example in examples:
                   self.trainExamplesHistory.append((example['contiguousGameState'], example['pi'], example['reward']))
                 
            log.info('Loading done!')

            
            # examples based on the model were already collected (loaded)
            # self.skipFirstSelfPlay = True


    def saveModelInfo(self):
        modelFile = self.args.model_folder + 'model.json'
        if not os.path.isfile(modelFile):
            log.warning(f'File "{modelFile}" not found!')
            log.info('Creating model.json')
            
            nnargs_dict = dict(nnargs)
            modelargs_dict = dict(self.args)
            modelInfo = {
                    'nnargs': nnargs_dict,
                    'modelargs': modelargs_dict,
                    'iteration': self.iteration,
                    'timeSpent': 0 
                    }

            with open(modelFile, "w") as file:
                json.dump(modelInfo, file) 
        else:
            log.info('Found model.json')
            
