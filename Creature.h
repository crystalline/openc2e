/*
 *  Creature.h
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue May 25 2004.
 *  Copyright (c) 2004-2006 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#ifndef __CREATURE_H
#define __CREATURE_H

#include "Agent.h"
#include "genome.h"
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

class CreatureAgent;
class Creature;

class Creature {
protected:
	CreatureAgent *parent;
	shared_ptr<genomeFile> genome;
	
	// non-specific bits
	unsigned int variant;
	bool female;
	
	bool alive, asleep, dreaming, tickage;
	bool zombie;

	unsigned int ticks;
	unsigned int age; // in ticks
	lifestage stage;

	AgentRef attention;

	// linguistic stuff

	// drives
	// to-be-processed instincts
	// conscious flag? brain/motor enabled flags? flags for each 'faculty'?
	
	void processGenes();
	virtual void addGene(gene *);

public:
	Creature(shared_ptr<genomeFile> g, bool is_female, unsigned char _variant);
	void setAgent(CreatureAgent *a);
	virtual ~Creature();
	virtual void tick();

	virtual void ageCreature();
	lifestage getStage() { return stage; }

	unsigned int getVariant() { return variant; }
	void setAsleep(bool asleep);
	bool isAsleep() { return asleep; }
	void setDreaming(bool dreaming);
	bool isDreaming() { return dreaming; }
	bool isFemale() { return female; }
	bool isAlive() { return alive; }
	void setZombie(bool z) { zombie = z; }
	bool isZombie() { return zombie; }
	unsigned int getAge() { return age; }
	shared_ptr<genomeFile> getGenome() { return genome; }

	virtual unsigned int getGait() = 0;
	
	void born();
	void die();
};

// c1

struct c1Reaction {
	bioReactionGene *data;
	void init(bioReactionGene *);
};

struct c1Receptor {
	bioReceptorGene *data;
	unsigned char *locus;
	void init(bioReceptorGene *, class c1Creature *);
};

struct c1Emitter {
	bioEmitterGene *data;
	unsigned char *locus;
	void init(bioEmitterGene *, class c1Creature *);
};

class c1Creature : public Creature {
protected:
	// biochemistry
	unsigned char chemicals[256];
	std::vector<shared_ptr<c1Reaction> > reactions;
	std::vector<c1Receptor> receptors;
	std::vector<c1Emitter> emitters;
	
	// loci
	unsigned char floatingloci[8];
	unsigned char muscleenergy;
	unsigned char lifestageloci[7];
	unsigned char fertile, receptive, pregnant;
	unsigned char dead;
	unsigned char senses[6], involaction[8], gaitloci[8];
	unsigned char drives[16];

	unsigned int biochemticks;
	bioHalfLivesGene *halflives;

	void addGene(gene *);
	void tickBiochemistry();
	void processReaction(c1Reaction &);
	void processEmitter(c1Emitter &);
	void processReceptor(c1Receptor &);
	inline unsigned int calculateTickMask(unsigned char);
	inline unsigned int calculateMultiplier(unsigned char);

public:
	c1Creature(shared_ptr<genomeFile> g, bool is_female, unsigned char _variant);

	void tick();

	void addChemical(unsigned char id, unsigned char val);
	void subChemical(unsigned char id, unsigned char val);
	unsigned char getChemical(unsigned char id) { return chemicals[id]; }
	
	unsigned char getDrive(unsigned int id) { assert(id < 16); return drives[id]; }
	
	unsigned char *getLocusPointer(bool receptor, unsigned char o, unsigned char t, unsigned char l);

	unsigned int getGait();
};

// c2e

struct c2eReaction {
	bioReactionGene *data;
	float rate;
	unsigned int receptors;
	void init(bioReactionGene *);
};

struct c2eReceptor {
	bioReceptorGene *data;
	bool processed;
	float lastvalue;
	float *locus;
	unsigned int *receptors;
	float nominal, threshold, gain;
	void init(bioReceptorGene *, class c2eOrgan *, shared_ptr<c2eReaction>);
};

struct c2eEmitter {
	bioEmitterGene *data;
	unsigned char sampletick;
	float *locus;
	float threshold, gain;
	void init(bioEmitterGene *, class c2eOrgan *);
};

class c2eOrgan {
protected:
	friend struct c2eReceptor;
	friend struct c2eEmitter;
	
	class c2eCreature *parent;	
	organGene *ourGene;

	std::vector<shared_ptr<c2eReaction> > reactions;
	std::vector<c2eReceptor> receptors;
	std::vector<c2eEmitter> emitters;

	// data
	float energycost, atpdamagecoefficient;
	
	// variables
	float lifeforce, shorttermlifeforce, longtermlifeforce;
	
	// locuses
	float biotick, damagerate, repairrate, clockrate, injurytoapply;
	unsigned int clockratereceptors, repairratereceptors, injuryreceptors;

	void processReaction(c2eReaction &);
	void processEmitter(c2eEmitter &);
	void processReceptor(c2eReceptor &, bool checkchem);
	
	float *getLocusPointer(bool receptor, unsigned char o, unsigned char t, unsigned char l, unsigned int **receptors);

public:
	c2eOrgan(c2eCreature *p, organGene *g);
	void tick();

	float getClockRate() { return clockrate; }
	float getRepairRate() { return repairrate; }
	float getDamageRate() { return damagerate; }
	float getEnergyCost() { return energycost; }
	float getInjuryToApply() { return injurytoapply; }
	float getInitialLifeforce() { return lifeforce; }
	float getShortTermLifeforce() { return shorttermlifeforce; }
	float getLongTermLifeforce() { return longtermlifeforce; }
	float getATPDamageCoefficient() { return atpdamagecoefficient; }
	
	unsigned int getReceptorCount() { return receptors.size(); }
	unsigned int getEmitterCount() { return emitters.size(); }
	unsigned int getReactionCount() { return reactions.size(); }
	
	void applyInjury(float);
};

class c2eCreature : public Creature {
protected:
	// biochemistry
	std::vector<shared_ptr<c2eOrgan> > organs;
	float chemicals[256];

	// loci
	float lifestageloci[7];
	float muscleenergy;
	float floatingloci[32];
	float fertile, pregnant, ovulate, receptive, chanceofmutation, degreeofmutation;
	float dead;
	float senses[14], involaction[8], gaitloci[16];
	float drives[20];

	bioHalfLivesGene *halflives;

	class c2eBrain *brain;

	void tickBiochemistry();
	void addGene(gene *);

public:
	c2eCreature(shared_ptr<genomeFile> g, bool is_female, unsigned char _variant);

	void tick();

	void adjustChemical(unsigned char id, float value);
	float getChemical(unsigned char id) { return chemicals[id]; }
	void adjustDrive(unsigned int id, float value);
	float getDrive(unsigned int id) { assert(id < 20); return drives[id]; }

	unsigned int noOrgans() { return organs.size(); }
	shared_ptr<c2eOrgan> getOrgan(unsigned int i) { assert(i < organs.size()); return organs[i]; }
	
	class c2eBrain *getBrain() { return brain; }

	float *getLocusPointer(bool receptor, unsigned char o, unsigned char t, unsigned char l);
	
	unsigned int getGait();
};

#endif

/* vim: set noet: */
